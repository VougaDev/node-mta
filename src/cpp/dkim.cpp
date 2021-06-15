
#include "dkim.hpp"

namespace dkim
{

  Canonicalization::Canonicalization(
      const std::string &name,
      std::function<std::string(const std::string &, const std::string &)> forHeader,
      std::function<std::string(std::string &)> forBody) : name{name}, forHeader{forHeader}, forBody{forBody}
  {
  }
  Canonicalization CanonicalizationType::relax;
  Canonicalization CanonicalizationType::simple;
  Canonicalization *CanonicalizationType::RELAXED()
  {
    if (CanonicalizationType::relax.name.empty())
      CanonicalizationType::relax = Canonicalization(
          "relaxed",
          [](const std::string &name, const std::string &value) {
            return model::util::tolower(model::util::trim(name)).append(":").append(model::util::trim(model::util::replaceRegex(value, "[ \t\n\f\r]+", " ")));
          },
          [](std::string &body) {
            body = model::util::replaceRegex(body, "[ \t\n\f\r]+\r\n", "\r\n");
            body = model::util::replaceRegex(body, "[ \t]+", " ");
            while (body.at(body.length() - 1) != '\n' && body.at(body.length() - 2) != '\r')
            {
              body.erase(body.length() - 2, 2);
            }
            body.append("\r\n");
            return body;
          });
    return &CanonicalizationType::relax;
  }

  Canonicalization *CanonicalizationType::SIMPLE()
  {
    if (CanonicalizationType::simple.name.empty())
      CanonicalizationType::simple = Canonicalization(
          "simple",
          [](const std::string &name, const std::string &value) {
            return std::string(name).append(": ").append(value);
          },
          [](std::string &body) {
            while (body.length() > 1 && body.at(body.length() - 1) == '\n' && body.at(body.length() - 2) == '\r')
            {
              body.erase(body.length() - 2, 2);
            }
            body.append("\r\n");
            return body;
          });
    return &CanonicalizationType::simple;
  }

  RSA *SigningAlgorithmType::privateKey = NULL;
  SigningAlgorithm SigningAlgorithmType::rsa_sha1;
  SigningAlgorithm SigningAlgorithmType::rsa_sha256;

  SigningAlgorithm::SigningAlgorithm(
      const std::string &notation,
      std::function<std::string(const std::string &)> hash,
      std::function<std::string(const std::string &)> sign) : notation{notation}, hash{hash}, sign{sign} {};

  int SigningAlgorithmType::readPrivateKey(const model::config &configuration)
  {
    if (SigningAlgorithmType::privateKey == NULL)
    {
      if (SigningAlgorithmType::readPrivateKey(
              &SigningAlgorithmType::privateKey,
              (unsigned char *)configuration.privateKey.c_str(),
              configuration.privateKey.length()) == -1)
      {
        printf(" * ERROR loading rsa key\n");
        return 0;
      }
    }
    return 1;
  }

  int SigningAlgorithmType::readPrivateKey(RSA **rsa, unsigned char *buff, int len)
  {
    BIO *mem;

    if ((mem = BIO_new_mem_buf(buff, -1)) == NULL)
    {
      return -1;
    }

    *rsa = PEM_read_bio_RSAPrivateKey(mem, rsa, NULL, NULL);
    BIO_free(mem);

    if (*rsa == NULL)
    {
      return -1;
    }
    return 0;
  }

  SigningAlgorithm *SigningAlgorithmType::RSA_SHA256(const model::config &configuration)
  {
    if (SigningAlgorithmType::rsa_sha256.notation.empty())
    {
      if (SigningAlgorithmType::readPrivateKey(configuration))
      {
        SigningAlgorithmType::rsa_sha256 = SigningAlgorithm(
            "rsa-sha256",
            [](const std::string &text) {
              unsigned char hash[SHA256_DIGEST_LENGTH];
              SHA256((unsigned char *)text.c_str(), text.length(), hash);
              return model::base64::encode((unsigned char const *)hash, SHA256_DIGEST_LENGTH);
            },
            [](const std::string &text) {
              unsigned char hash[SHA256_DIGEST_LENGTH];
              SHA256((unsigned char *)text.c_str(), text.length(), hash);
              size_t length = RSA_size(SigningAlgorithmType::privateKey);
              unsigned char signature[2 * length];
              unsigned int sig_len;
              if (RSA_sign(
                      NID_sha256,
                      (unsigned char *)hash,
                      SHA256_DIGEST_LENGTH,
                      signature, &sig_len,
                      SigningAlgorithmType::privateKey) > 0)
              {
                return model::base64::encode((unsigned char const *)signature, sig_len);
              }
              else
              {
                printf(" * ERROR RSA_sign(): %s\n", ERR_error_string(ERR_get_error(), NULL));
              }
              return std::string("");
            });
      }
    }
    return &SigningAlgorithmType::rsa_sha256;
  }

  SigningAlgorithm *SigningAlgorithmType::RSA_SHA1(const model::config &configuration)
  {
    if (SigningAlgorithmType::rsa_sha1.notation.empty())
    {
      if (SigningAlgorithmType::readPrivateKey(configuration))
      {
        SigningAlgorithmType::rsa_sha1 = SigningAlgorithm(
            "rsa-sha1",
            [](const std::string &text) {
              unsigned char hash[SHA_DIGEST_LENGTH];
              SHA1((unsigned char *)text.c_str(), text.length(), hash);
              return model::base64::encode((unsigned char const *)hash, SHA_DIGEST_LENGTH);
            },
            [](const std::string &text) {
              std::string hash = SigningAlgorithmType::rsa_sha1.hash(text);
              size_t length = RSA_size(SigningAlgorithmType::privateKey);
              unsigned char signature[length];
              unsigned int sig_len;
              if (RSA_sign(
                      NID_sha1,
                      (unsigned char *)hash.c_str(),
                      SHA_DIGEST_LENGTH,
                      signature, &sig_len,
                      SigningAlgorithmType::privateKey) > 0)
              {
                printf(" * Base64 encoding the signature...\n");
                return model::base64::encode((unsigned char const *)signature, sig_len);
              }
              else
              {
                printf(" * ERROR RSA_sign(): %s\n", ERR_error_string(ERR_get_error(), NULL));
              }
              return std::string("");
            });
      }
    }
    return &SigningAlgorithmType::rsa_sha1;
  }

  std::string signer::serialize(std::vector<std::pair<std::string, std::string>> &signatureData) const
  {
    std::string ss;
    std::string temp;
    size_t length = 0;
    for (size_t i = 0; i < signatureData.size(); i++)
    {
      auto &entry = signatureData.at(i);
      temp.clear();
      temp.append(entry.first).append("=").append(entry.second).append(";");
      if (temp.length() + length + 1 > MAX_HEADER_LENGTH)
      {
        length = temp.length();
        ss.append("\r\n\t").append(temp);
      }
      else
      {
        if (i > 0)
        {
          ss.append(" ");
        }
        ss.append(temp);
        length += 1 + temp.length();
      }
    }
    ss.append("\r\n\t").append("b=");
    return ss;
  }

  signer::signer(
      model::config &configuration,
      Canonicalization *headerCanon,
      Canonicalization *bodyCanon,
      SigningAlgorithm *algorithm) : configuration{&configuration}, headerCanon{headerCanon}, bodyCanon{bodyCanon}, algorithm{algorithm} {}

  std::string signer::hashBody(const model::email *mail) const
  {
    std::string cBody = mail->getBodyPart();
    cBody = bodyCanon->forBody(cBody);
    // signatureData.emplace_back("l",std::to_string(cBody.length()));
    return algorithm->hash(cBody);
  }
  void signer::sign(const model::email *mail, const std::string *bodyHash) const
  {
    const bool notSendingToList = mail->list.address.empty();
    if (notSendingToList || mail->dkimHeader.empty())
    {

      time_t ts = time(NULL);
      std::vector<std::pair<std::string, std::string>> signatureData;

      signatureData.emplace_back("v", "1");
      signatureData.emplace_back("a", algorithm->notation);
      signatureData.emplace_back("q", "dns/txt");
      signatureData.emplace_back("c", std::string(headerCanon->name).append("/").append(bodyCanon->name));
      signatureData.emplace_back("t", std::to_string(ts));
      signatureData.emplace_back("s", configuration->dkimSelector);
      signatureData.emplace_back("d", configuration->hostname);

      std::stringstream names;
      std::stringstream values;
      std::stringstream headers;

      for (size_t i = 0; i < mail->headers.size(); i++)
      {
        const model::header *header = &mail->headers.at(i);

        if (header->key == "To")
        {
          if(notSendingToList){
            header->value = mail->to[mail->recipientIndex].toString();
          }else {
            header->value = mail->list.toString();
          }
        }
        if (header->sign)
        {
          names << header->key;
          if (i < mail->headers.size() - 1)
          {
            names << ":";
          }
          values << headerCanon->forHeader(header->key, header->value) << "\r\n";
        }
        headers << header->key << ":" << header->value << "\r\n";
      }
      mail->headerPart = headers.str();
      signatureData.emplace_back("h", names.str());
      signatureData.emplace_back("bh", *bodyHash);

      std::string serializedSignature = serialize(signatureData);
      values << headerCanon->forHeader("DKIM-Signature", serializedSignature);
      std::string signature = algorithm->sign(values.str());
      mail->dkimHeader = serializedSignature.append(model::util::fold(signature, MAX_HEADER_LENGTH, 3));
    }
  }
} // namespace dkim