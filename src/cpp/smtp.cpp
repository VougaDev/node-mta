#include "smtp.hpp"

namespace smtp
{
  sender::sender() = default;
  sender::~sender() = default;

  int sender::createConnection(std::string &mxHost)
  {

    int sd = socket(PF_INET, SOCK_STREAM, 0);

    struct hostent *host;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    // get from host or ip
    if ((signed)inet_addr(mxHost.c_str()) == -1)
    {
      if ((host = gethostbyname(mxHost.c_str())) == NULL)
      {
        log::error("gethostbyname() failed - 1 ");
        return -1;
      }

      struct in_addr **addr_list;
      addr_list = (struct in_addr **)host->h_addr_list;
      for (int i = 0; addr_list[i] != NULL; i++)
      {
        //strcpy(ip , inet_ntoa(*addr_list[i]) );
        addr.sin_addr = *addr_list[i];
        if (logLevel == LogLevel::VERBOSE)
        {
          std::cout << mxHost << " resolved to " << inet_ntoa(*addr_list[i]) << std::endl;
        }
        inet_pton(AF_INET, inet_ntoa(*addr_list[i]), &addr.sin_addr);
        break;
      }
      setTimeOut(sd, timeout);
    }
    else
    {
      addr.sin_addr.s_addr = inet_addr(mxHost.c_str());
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(configuration->port);
    if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      close(sd);
      perror(mxHost.c_str());
      log::error("connection attempt failed ");
      return -1;
      // abort();
    }

    return sd;
  }

  void sender::setTimeOut(const int &socket, int seconds)
  {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
      log::warn("Failed to set socket time out - 1 ");
    }
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
      log::warn("Failed to set socket time out - 2 ");
    }
  }

  bool sender::send(const model::email &mail, std::string &mxHost)
  {
    const int connection = createConnection(mxHost);
    if (connection <= 0)
    {
      if (logLevel > LogLevel::NONE)
      {
        log::error("Failed to connect to remote host");
        return false;
      }
    }
    handler h(connection, mail, configuration, signer);
    return h.sayHello();
  }

  handler::handler(const int server, const model::email &mail, model::config *configuration, dkim::signer *signer) : server{server},
                                                                                                                     configuration{configuration},
                                                                                                                     mail{&mail},
                                                                                                                     signer{signer}
  {
  }

  void handler::reset()
  {
    memset(buffer, 0, BUFFER_SIZE);
    buffer[0] = '\0';
    wrapper.clear();
  }
  bool handler::failure(const std::string &value)
  {
    wrapper.clear();
    wrapper.append("Failed to send command --> ").append(value);
    log::error(wrapper.c_str());
    release();
    return false;
  }

  bool handler::failure(const std::string &value, const std::string &response)
  {
    std::string res("Failure after command --> ");
    res.append(value)
        .append("\n\t - Server Response --> ")
        .append(response);
    log::error(res.c_str());
    release();
    return false;
  }

  bool handler::init()
  {
    result = read(server, buffer, BUFFER_SIZE);
    wrapper.clear();
    wrapper.append("[Server] init [")
        .append(std::to_string(result))
        .append("] ")
        .append(buffer);
    log::debug(wrapper.c_str());
    return true;
  }

  bool handler::sayHello()
  {
    reset();
    const std::string value = std::string("EHLO ").append(configuration->hostname).append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = send(server, value.c_str(), value.length(), 0);
    if (result < 0)
      return failure(value);

    result = read(server, buffer, BUFFER_SIZE);
    wrapper.clear();
    wrapper.append(buffer);
    while (!model::util::contains(wrapper, "250 "))
    {
      result = read(server, buffer, BUFFER_SIZE);
      wrapper.clear();
      wrapper.append(buffer);
      if (model::util::contains(wrapper, "501 ") ||
          model::util::contains(wrapper, "503 "))
      {
        return failure(value, wrapper);
      }
    }
    wrapper.clear();
    wrapper.append(buffer);
    if (!model::util::contains(wrapper, "STARTTLS"))
    {
      log::error(std::string("Remote host not supporting TLS - Closing connection").c_str());
      return failure(value, wrapper);
    }
    return startTLS();
  }

  void handler::initCtx()
  {
    const SSL_METHOD *method;
    SSL_library_init();
    SSL_load_error_strings();     // Bring in and register error messages
    OpenSSL_add_all_algorithms(); // Load cryptos

    method = SSLv23_client_method(); // Create new client-method instance
    ctx = SSL_CTX_new(method);
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3); // Create new context
    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1); // Create new context
  }

  bool handler::sslOk()
  {
    const int err = SSL_get_error(ssl, result);
    // const int st = ERR_get_error();
    if (err == SSL_ERROR_NONE)
      return true;
    else if (err == SSL_ERROR_WANT_READ)
    {
      log::error(std::string("Error SSL_ERROR_WANT_READ failure --> ").c_str());
      SSL_shutdown(ssl);
    }
    else if (SSL_ERROR_SYSCALL)
    {
      log::error(std::string("Error SSL_ERROR_SYSCALL failure --> ").c_str());
      SSL_shutdown(ssl);
    }
    return false;
  }

  bool handler::startTLS()
  {
    reset();
    const std::string value = std::string("STARTTLS").append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = send(server, value.c_str(), value.length(), 0);
    if (result < 0)
      return failure(value);
    result = read(server, buffer, BUFFER_SIZE);
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "220"))
    {
      log::error(std::string("Remote host not supporting STARTTLS command - Closing connection").c_str());
      return failure(value, wrapper);
    }
    initCtx();
    if (ctx == NULL)
    {
      ERR_print_errors_fp(stderr);
      {
        log::error(std::string("SSL Context initialization failure").c_str());
        return false;
      }
    }
    ssl = SSL_new(ctx);               // create new SSL connection
    result = SSL_set_fd(ssl, server); // attach the socket descriptor
    if (!sslOk())
      return false;
    result = SSL_connect(ssl);
    if (result == FAILURE)
    {
      log::error(std::string("SSL conneciont failure").c_str());
      return false;
    }
    return sayHelloWithSSL();
  }

  bool handler::sayHelloWithSSL()
  {
    reset();
    log::info("SSL Connection success !");
    const std::string value = std::string("EHLO ").append(configuration->hostname).append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = SSL_write(ssl, value.c_str(), value.length());
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "250"))
    {
      return failure(value, wrapper);
    }
    
    std::string bodyHash = this->signer->hashBody(mail);
    for (size_t index = 0; index < mail->to.size(); index++)
    {
      mail->recipientIndex = index;
      signer->sign(mail,&bodyHash);
      if (!sendMailFrom())
      {
        release();
        return false;
      }
    }
    return sendQuit();
  }

  bool handler::sendMailFrom()
  {
    reset();
    std::string value = std::string("MAIL FROM: <").append(mail->from.address).append(">").append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = SSL_write(ssl, value.c_str(), value.length());
    if (result < 1)
    {
      return failure(value);
    }
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "250"))
    {
      return failure(value, wrapper);
    }
    return sendRcptTo();
  }

  bool handler::sendRcptTo()
  {
    reset();
    std::string value = std::string("RCPT TO: <").append(mail->to[mail->recipientIndex].address).append(">").append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = SSL_write(ssl, value.c_str(), value.length());
    if (result < 1)
    {
      return failure(value);
    }
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "250"))
    {
      return failure(value, wrapper);
    }
    return sendData();
  }

  bool handler::sendData()
  {
    reset();
    // We are going to send data
    std::string value = std::string("DATA").append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = SSL_write(ssl, value.c_str(), value.length());
    if (result < 1)
    {
      return failure(value);
    }
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "354"))
    {
      return failure(value, wrapper);
    }

    // We are sending data
    std::ostringstream oss;
    oss << *mail << CRLF << DOT << CRLF;
    std::string message = oss.str();
    log::debug("Sending message body\n");
    result = SSL_write(ssl, message.c_str(), message.length());
    if (result < 1)
    {
      return failure(message);
    }
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    if (!model::util::contains(wrapper, "250"))
    {
      return failure(value, wrapper);
    }
    return true;
  }

  bool handler::sendQuit()
  {
    std::string value = std::string("QUIT").append(CRLF);
    log::debug(std::string("[Client] ").append(value).c_str());
    result = SSL_write(ssl, value.c_str(), value.length());
    if (result < 1)
    {
      return failure(value);
    }
    bytesCount = SSL_read(ssl, buffer, BUFFER_SIZE);
    buffer[bytesCount] = 0;
    wrapper.clear();
    wrapper.append(buffer);
    if (!model::util::contains(wrapper, "221"))
    {
      return failure(value, wrapper);
    }
    log::debug(std::string("[Server] ").append(wrapper).c_str());
    return release();
  }

  bool handler::release()
  {
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(server);
    return true;
  }
} // namespace smtp
