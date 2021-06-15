#ifndef M_DKIM_H
#define M_DKIM_H
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/sha.h>

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

#include <unordered_map>
#include <utility>
#include <string>
#include <sstream>
#include <functional>
#include <regex>
#include "model.hpp"

namespace dkim
{
  const static size_t MAX_HEADER_LENGTH = 67;
  class signer;
  class SigningAlgorithmType;
  class CanonicalizationType;
  class Canonicalization
  {
  public: //del
    std::string name;
    std::function<std::string(const std::string &, const std::string &)> forHeader;
    std::function<std::string(std::string &)> forBody;
    Canonicalization() = default;
    Canonicalization(
        const std::string &name,
        std::function<std::string(const std::string &, const std::string &)> forHeader,
        std::function<std::string(std::string &)> forBody);
    friend class signer;
    friend class SigningAlgorithmType;
    friend class CanonicalizationType;
  };

  class CanonicalizationType
  {
  private:
    static Canonicalization relax;
    static Canonicalization simple;

  public:
    static Canonicalization *RELAXED();
    static Canonicalization *SIMPLE();
  };

  struct SigningAlgorithm
  {
  public:
    std::string notation;
    std::function<std::string(const std::string &)> hash;
    std::function<std::string(const std::string &)> sign;
    friend class signer;
    friend class SigningAlgorithmType;

  public:
    SigningAlgorithm() = default;
    SigningAlgorithm(
        const std::string &notation,
        std::function<std::string(const std::string &)> hash,
        std::function<std::string(const std::string &)> sign
    );
  };

  class SigningAlgorithmType
  {
  public:
    static RSA *privateKey;
    static SigningAlgorithm rsa_sha1;
    static SigningAlgorithm rsa_sha256;
    static int readPrivateKey(const model::config &configuration);
    static int readPrivateKey(RSA **rsa, unsigned char *buff, int len);

  public:
    static SigningAlgorithm *RSA_SHA256(const model::config &configuration);
    static SigningAlgorithm *RSA_SHA1(const model::config &configuration);
  };

  class signer
  {
  private:
    model::config *configuration;
    Canonicalization *headerCanon;
    Canonicalization *bodyCanon;
    SigningAlgorithm *algorithm;

    std::string serialize(std::vector<std::pair<std::string, std::string>> &signatureData) const;

  public:
    signer() = default;
    signer(
        model::config &configuration,
        Canonicalization *headerCanon,
        Canonicalization *bodyCanon,
        SigningAlgorithm *algorithm);
    std::string hashBody(const model::email * mail) const;
    void sign(const model::email * mail, const std::string * bodyHash) const;
  };
} // namespace dkim

#endif //M_DKIM_H