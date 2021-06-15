#ifndef BOUTCH_SMTP_H
#define BOUTCH_SMTP_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <fstream>

#define BUFFER_SIZE 8192
#define CRLF "\r\n"
#define DOT "."
#define FAILURE -1

#include "model.hpp"
#include "dkim.hpp"

namespace smtp
{
  enum LogLevel
  {
    NONE,
    INFO,
    VERBOSE
  };

  struct log
  {
    static void info(const char *log)
    {
      std::cout <<"[INFO] - "<< log << std::endl;
    }
    static void debug(const char *log)
    {
      std::cout <<"[DEBUG] - " << log << std::endl;
    }
    static void warn(const char *log)
    {
      std::cout <<"[WARN] - " << log << std::endl;
    }

    static void error(const char *log)
    {
      std::cerr <<"[ERROR] - " << log << std::endl;
    }
  };

  class sender
  {
  private:
    model::config *configuration;
    dkim::signer * signer;
    int timeout = 15; // seconds
    int logLevel = LogLevel::NONE;
    int createConnection(std::string &mxHost);
    void setTimeOut(const int &socket, int seconds);

  public:
    sender(model::config &configuration, dkim::signer &signer) : configuration{&configuration}, signer{&signer} {}
    sender();
    ~sender();
    void setDebugEnabled(const bool &enabled);
    bool send(const model::email &email, std::string &mxHost);
  };

  class handler
  {
    SSL_CTX *ctx;
    SSL *ssl;
    char buffer[BUFFER_SIZE];
    std::string wrapper;
    int server;
    int result;
    int bytesCount;
    const model::config *configuration;
    const model::email *mail;
    const dkim::signer *signer;

    handler(const int server, const model::email &mail, model::config *configuration, dkim::signer *signer);
    bool init();
    void initCtx();
    bool sslOk();
    void reset();
    bool failure(const std::string &value);
    bool failure(const std::string &value, const std::string &response);
    bool sayHello();
    bool sayHelloWithSSL();
    bool startTLS();
    bool sendMailFrom();
    bool sendRcptTo();
    bool sendData();
    bool sendQuit();
    bool release();
    friend sender;
  };

} // namespace smtp

#endif // BOUTCH_SMTP_H