#ifndef MODELS_H
#define MODELS_H

#include <unordered_map>
#include <vector>
#include <string>
#include <string.h>
#include <sys/time.h>
#include <ostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <regex>

namespace model
{

  enum FileType
  {
    INLINE,
    ATTACHMENT,
    BOTH,
    NONE
  };
  struct base64
  {
  private:
    static const std::string chars;

  public:
    static std::string encode(unsigned char const *, unsigned int len);
    static std::string encode(std::string str, unsigned int len);
    static std::string encode(std::string str);
    static std::string decode(std::string const &s);
    static inline bool is_base64(unsigned char c);
  };

  struct util
  {
  private:
    static const char MimeTypes[][2][128];

  public:
    static std::string replaceAll(std::string &s, const std::string &search, const std::string &replace);
    static bool contains(const std::string &str, const std::string &search);
    static std::vector<std::string> split(const std::string &str, const std::string &delim);
    static std::string date(bool utc);
    static std::string date_rfc2822();
    static std::string fileBasename(const std::string &path);
    static std::string getFileContent(const std::string &path);
    static std::string get_file_contents(const char *filename);
    static long int microseconds();
    static const char *getMimeType(char *szFileExt);
    static std::string getFileExtension(const std::string &FileName);
    static std::string wrapLines(const std::string &text, const size_t &maxLength);
    static std::string trim(const std::string &str);
    static std::string tolower(std::string s);
    static std::string replaceRegex(const std::string &text, const std::string &regex, const std::string &replacement);
    static std::string fold(const std::string &text, size_t maxLength = 78, size_t offset = 3);
  };

  struct header
  {

  public:
    bool sign;
    std::string key;
    mutable std::string value;
    header(const std::string &key, const std::string &value,const bool & sign) : sign{sign},key{key}, value{value} {}
    
  };

  struct config
  {
    std::string hostname;
    size_t port;
    std::string privateKey;
    std::string publicKey;
    std::string dkimSelector;
  };

  struct address
  {
  public:
    std::string name;
    std::string address;
    std::string toString() const
    {
      if (name.empty())
        return std::string("<").append(address).append(">");
      return std::string("\"").append(name).append("\" <").append(address).append(">");
    }
  };

  struct file
  {
    std::string id;
    std::string path;
    std::string name;
    bool inLine;
    file(std::string &id, std::string &path, std::string &name, bool inLine) : id{id}, path{path}, name{name}, inLine{inLine} {}
  };

  struct email
  {
  public:
    static const std::string delim_mixed;
    static const std::string delim_related;
    static const std::string delim_alternative;
    FileType type;
    config * configuration;
    std::string id;
    address from;
    /**
     * When sending bulk email, we do not want all the recipients to know 
     * email addresses of each others, in order to achieve this we use a list address
     * email address. 
     */
    address list;
    std::vector<address> to;
    std::string subject;
    std::string html;
    std::string text;
    std::string replyTo;
    std::string returnPath;
    std::string encoding = "UTF-8";
    std::string date = util::date_rfc2822();
    std::vector<file> files;
    std::vector<header> headers;
    mutable std::string dkimHeader;
    mutable std::string bodyPart;
    mutable std::string headerPart;
    mutable size_t recipientIndex = 0;
    email(config &configuration) : configuration{&configuration},
                                         id{std::string("<").append(std::to_string(util::microseconds())).append("@").append(configuration.hostname).append(">")} {};
    std::string getBodyPart() const;
    void prepare();
  };
  std::ostream &operator<<(std::ostream &os, const email &mail);
  std::ostream &operator<<(std::ostream &os, const address &a);

} // namespace model

#endif //MODELS_H