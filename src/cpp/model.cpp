#include "model.hpp"

namespace model
{

  std::ostream &operator<<(std::ostream &os, const address &a)
  {
    if (a.name.empty())
      return os << std::string("<").append(a.address).append(">");

    return os << std::string("\"").append(a.name).append("\" <").append(a.address).append(">");
  }
  std::ostream &operator<<(std::ostream &os, const config &c)
  {
    os << "Hostname   : " << c.hostname << std::endl
       << "Port       : " << c.port << std::endl
       << "PrivateKey : " << c.privateKey << std::endl;
    return os;
  }

  const std::string email::delim_mixed = "VOUGADEVMIXED1001";
  const std::string email::delim_related = "VOUGADEVRELATED1002";
  const std::string email::delim_alternative = "VOUGADEVALTERNATIVE1000";

  std::string email::getBodyPart() const
  {
    /**
     * @see https://stackoverflow.com/questions/30351465/html-email-with-inline-attachments-and-non-inline-attachments
     */
    if (!this->bodyPart.empty())
      return this->bodyPart;
    std::stringstream attachments;
    std::stringstream inlines;
    std::stringstream ss;
    ss << "--" << email::delim_alternative << "\r\n"
       << "Content-Type: text/plain; charset=\"" << this->encoding << "\"\r\n"
       << "Content-Transfer-Encoding: base64\r\n\r\n"
       << (this->text.empty() ? util::fold(base64::encode(util::replaceRegex(util::replaceRegex(this->html, "(<[/]?br[/]?>+)", "\r\n"), "(<[^<>]+>)", "")), 76, 0) : this->text)
       << "\r\n\r\n"
       << "--" << email::delim_alternative << "\r\n";
    if (this->type != FileType::INLINE)
    {
      ss << "Content-Type: multipart/mixed; boundary=\"" << email::delim_mixed << "\"\r\n\r\n"
         << "--" << email::delim_mixed << "\r\n";
    }
    if (this->type == FileType::INLINE || this->type == FileType::BOTH)
    {
      ss << "Content-Type: multipart/related; boundary=\"" << email::delim_related << "\"\r\n\r\n"
         << "--" << email::delim_related << "\r\n";
    }
    ss << "Content-Type: text/html; charset=\"" << this->encoding << "\"\r\n"
       << "Content-Transfer-Encoding: base64\r\n\r\n"
       << util::fold(base64::encode(this->html), 76, 0) << "\r\n\r\n";
    if (this->files.size() > 0)
    {
      size_t fileCount = this->files.size();
      std::stringstream *ss;
      for (size_t i = 0; i < fileCount; i++)
      {
        model::file f = this->files.at(i);
        ss = f.inLine ? &inlines : &attachments;
        std::string filename = util::fileBasename(f.path);
        std::string fc = base64::encode(util::get_file_contents(f.path.c_str()));
        std::string extension = util::getFileExtension(filename);
        if (!f.name.empty())
        {
          filename = f.name;
        }
        const char *mimetype = util::getMimeType((char *)extension.c_str());
        // cout << "MIME " << mimetype << endl << extension << endl;
        // cout << "FILE CONTENT " << fc << endl;
        if (f.inLine)
        {
          *ss << "--" << email::delim_related << "\r\n";
        }
        else
        {
          *ss << "--" << email::delim_mixed << "\r\n";
        }

        *ss << "Content-Type: " << mimetype << "; name=\"" << filename << "\"\r\n"
            << "Content-Transfer-Encoding: base64\r\n"
            << "Content-Disposition: " << (f.inLine ? "inline" : "attachment") << "; filename=\""
            << filename << "\"\r\n"
            << "Content-ID: <" << f.id << "@" << this->configuration->hostname << ">\r\n"
            << "X-Attachment-Id: " << f.id << "@" << this->configuration->hostname << "\r\n\r\n"
            << util::fold(fc, 76, 0) << "\r\n";
      }
    }
    if (this->type == FileType::BOTH)
    {
      inlines << "--" << email::delim_related << "--";
    }
    ss << inlines.str() << "\r\n"
       << attachments.str();
    if (this->type != FileType::INLINE)
    {
      ss << "--" << email::delim_mixed << "--";
    }
    else
    {
      ss << "--" << email::delim_related << "--";
    }
    ss << "\r\n"
       << "--" << email::delim_alternative << "--\r\n";
    this->bodyPart = ss.str();
    return this->bodyPart;
  }

  void email::prepare()
  {
    this->subject = std::string("=?").append(this->encoding).append("?B?").append(util::fold(base64::encode(this->subject),72,11)).append("?=");
    
  }

  std::ostream &operator<<(std::ostream &os, const email &mail)
  {

    os << "DKIM-Signature: " << mail.dkimHeader << "\r\n"
       << mail.headerPart
       << "Content-Type: multipart/alternative; boundary=\"" << email::delim_alternative << "\"\r\n\r\n"
       << mail.getBodyPart();
    return os;
  }

  const std::string base64::chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "abcdefghijklmnopqrstuvwxyz"
                                    "0123456789+/";
  const char util::MimeTypes[][2][128] =
      {
          {"***", "application/octet-stream"},
          {"csv", "text/csv"},
          {"tsv", "text/tab-separated-values"},
          {"tab", "text/tab-separated-values"},
          {"html", "text/html"},
          {"htm", "text/html"},
          {"doc", "application/msword"},
          {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
          {"ods", "application/x-vnd.oasis.opendocument.spreadsheet"},
          {"odt", "application/vnd.oasis.opendocument.text"},
          {"rtf", "application/rtf"},
          {"sxw", "application/vnd.sun.xml.writer"},
          {"txt", "text/plain"},
          {"xls", "application/vnd.ms-excel"},
          {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
          {"pdf", "application/pdf"},
          {"ppt", "application/vnd.ms-powerpoint"},
          {"pps", "application/vnd.ms-powerpoint"},
          {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
          {"wmf", "image/x-wmf"},
          {"atom", "application/atom+xml"},
          {"xml", "application/xml"},
          {"json", "application/json"},
          {"js", "application/javascript"},
          {"ogg", "application/ogg"},
          {"ps", "application/postscript"},
          {"woff", "application/x-woff"},
          {"xhtml", "application/xhtml+xml"},
          {"xht", "application/xhtml+xml"},
          {"zip", "application/zip"},
          {"gz", "application/x-gzip"},
          {"rar", "application/rar"},
          {"rm", "application/vnd.rn-realmedia"},
          {"rmvb", "application/vnd.rn-realmedia-vbr"},
          {"swf", "application/x-shockwave-flash"},
          {"au", "audio/basic"},
          {"snd", "audio/basic"},
          {"mid", "audio/mid"},
          {"rmi", "audio/mid"},
          {"mp3", "audio/mpeg"},
          {"aif", "audio/x-aiff"},
          {"aifc", "audio/x-aiff"},
          {"aiff", "audio/x-aiff"},
          {"m3u", "audio/x-mpegurl"},
          {"ra", "audio/vnd.rn-realaudio"},
          {"ram", "audio/vnd.rn-realaudio"},
          {"wav", "audio/x-wave"},
          {"wma", "audio/x-ms-wma"},
          {"m4a", "audio/x-m4a"},
          {"bmp", "image/bmp"},
          {"gif", "image/gif"},
          {"jpe", "image/jpeg"},
          {"jpeg", "image/jpeg"},
          {"jpg", "image/jpeg"},
          {"jfif", "image/jpeg"},
          {"png", "image/png"},
          {"svg", "image/svg+xml"},
          {"tif", "image/tiff"},
          {"tiff", "image/tiff"},
          {"ico", "image/vnd.microsoft.icon"},
          {"css", "text/css"},
          {"bas", "text/plain"},
          {"c", "text/plain"},
          {"h", "text/plain"},
          {"rtx", "text/richtext"},
          {"mp2", "video/mpeg"},
          {"mpa", "video/mpeg"},
          {"mpe", "video/mpeg"},
          {"mpeg", "video/mpeg"},
          {"mpg", "video/mpeg"},
          {"mpv2", "video/mpeg"},
          {"mov", "video/quicktime"},
          {"qt", "video/quicktime"},
          {"lsf", "video/x-la-asf"},
          {"lsx", "video/x-la-asf"},
          {"asf", "video/x-ms-asf"},
          {"asr", "video/x-ms-asf"},
          {"asx", "video/x-ms-asf"},
          {"avi", "video/x-msvideo"},
          {"3gp", "video/3gpp"},
          {"3gpp", "video/3gpp"},
          {"3g2", "video/3gpp2"},
          {"movie", "video/x-sgi-movie"},
          {"mp4", "video/mp4"},
          {"wmv", "video/x-ms-wmv"},
          {"webm", "video/webm"},
          {"m4v", "video/x-m4v"},
          {"flv", "video/x-flv"}};

  long int util::microseconds()
  {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
  }

  std::string util::trim(const std::string &str)
  {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
      return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
  }
  std::string util::tolower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(),
                   // static_cast<int(*)(int)>(std::tolower)         // wrong
                   // [](int c){ return std::tolower(c); }           // wrong
                   // [](char c){ return std::tolower(c); }
                   // wrong
                   [](unsigned char c) { return std::tolower(c); } // correct
    );
    return s;
  }

  std::string util::replaceAll(std::string &s, const std::string &search, const std::string &replace)
  {
    for (size_t pos = 0;; pos += replace.length())
    {
      // Locate the substring to replace
      pos = s.find(search, pos);
      if (pos == std::string::npos)
        break;
      // Replace by erasing and inserting
      s.erase(pos, search.length());
      s.insert(pos, replace);
    }
    return s;
  }

  bool util::contains(const std::string &str, const std::string &search)
  {
    std::size_t found = str.find(search);
    if (found != std::string::npos)
    {
      return 1;
    }
    return 0;
  }

  std::string util::getFileExtension(const std::string &FileName)
  {
    if (FileName.find_last_of(".") != std::string::npos)
      return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
  }

  const char *util::getMimeType(char *szFileExt)
  {
    // cout << "EXT " << szFileExt;
    for (unsigned int i = 0; i < sizeof(MimeTypes) / sizeof(MimeTypes[0]); i++)
    {
      if (strcmp(MimeTypes[i][0], szFileExt) == 0)
      {
        return MimeTypes[i][1];
      }
    }
    return MimeTypes[0][1]; //if does not match any,  "application/octet-stream" is returned
  }

  std::string util::fileBasename(const std::string &path)
  {
    std::string filename = path.substr(path.find_last_of("/\\") + 1);
    return filename;
    // without extension
    // std::string::size_type const p(base_filename.find_last_of('.'));
    // std::string file_without_extension = base_filename.substr(0, p);
  }

  std::string util::getFileContent(const std::string &path)
  {
    //std::ifstream file(path);
    //std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //return content;
    return path;
  }

  std::string util::get_file_contents(const char *filename)
  {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
      std::string contents;
      in.seekg(0, std::ios::end);
      contents.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&contents[0], contents.size());
      in.close();
      return (contents);
    }
    throw(errno);
  }

  std::vector<std::string> util::split(const std::string &str, const std::string &delim)
  {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
      pos = str.find(delim, prev);
      if (pos == std::string::npos)
        pos = str.length();
      std::string token = str.substr(prev, pos - prev);
      if (!token.empty())
        tokens.push_back(token);
      prev = pos + delim.length();
      // cout << token << endl;
    } while (pos < str.length() && prev < str.length());
    return tokens;
  }
  std::string util::date(bool utc)
  {
    time_t now = time(0);
    char *dt = ctime(&now);
    if (utc)
    {
      tm *gmtm = gmtime(&now);
      dt = asctime(gmtm);
    }
    return std::string(dt);
  }
  std::string util::date_rfc2822()
  {
    time_t current;
    char rfc_2822[40];
    time(&current);
    strftime(rfc_2822, sizeof(rfc_2822), "%a, %d %b %Y %T %z", localtime(&current));
    return std::string(rfc_2822);
  }
  std::string util::fold(const std::string &text, size_t maxLength, size_t offset)
  {
    size_t i = 0;
    i=0;
    bool tabulated = offset > 0;
    std::stringstream ss;
    while (true)
    {
      if (offset > 0 && text.length() - i > maxLength - offset)
      {
        ss << text.substr(i, maxLength - offset + 1);
        i += maxLength - offset + 1;
        offset = 0;
      }
      else if (text.length() - i > maxLength - 1)
      {
        ss << "\r\n" << (tabulated ? "\t": "") << text.substr(i, maxLength - 1);
        i += maxLength - 1;
      }
      else
      {
        ss << "\r\n" << (tabulated ? "\t": "") << text.substr(i);
        break;
      }
    }
    return ss.str();
  }
  std::string util::wrapLines(const std::string &text, const size_t &maxLength)
  {
    std::stringstream ss;
    size_t i;
    size_t lcount = 0;
    size_t len = text.length();
    for (i = 0; i < len; ++i)
    {
      ss << text.at(i);
      if (lcount == maxLength)
      {
        ss << "=\r\n";
        lcount = 0;
      }
      else
      {
        ++lcount;
      }
    }
    return ss.str();
  }

  std::string util::replaceRegex(const std::string &text, const std::string &regex, const std::string &replacement)
  {
    return std::regex_replace(text, std::regex(regex), replacement);
  }

#ifndef region_base64
  // ---------------------------------------- BASE64

  bool base64::is_base64(unsigned char c)
  {
    return (isalnum(c) || (c == '+') || (c == '/'));
  }

  std::string base64::encode(unsigned char const *bytes_to_encode, unsigned int in_len)
  {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
      char_array_3[i++] = *(bytes_to_encode++);
      if (i == 3)
      {
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (i = 0; (i < 4); i++)
          ret += chars[char_array_4[i]];
        i = 0;
      }
    }

    if (i)
    {
      for (j = i; j < 3; j++)
        char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

      for (j = 0; (j < i + 1); j++)
        ret += chars[char_array_4[j]];

      while ((i++ < 3))
        ret += '=';
    }

    return ret;
  }

  std::string base64::encode(std::string str, unsigned int in_len)
  {
    unsigned char const *bytes_to_encode = reinterpret_cast<const unsigned char *>(str.c_str());
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
      char_array_3[i++] = *(bytes_to_encode++);
      if (i == 3)
      {
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (i = 0; (i < 4); i++)
          ret += chars[char_array_4[i]];
        i = 0;
      }
    }

    if (i)
    {
      for (j = i; j < 3; j++)
        char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

      for (j = 0; (j < i + 1); j++)
        ret += chars[char_array_4[j]];

      while ((i++ < 3))
        ret += '=';
    }
    return ret;
  }

  std::string base64::encode(std::string str)
  {
    unsigned int in_len = str.length();
    unsigned char const *bytes_to_encode = reinterpret_cast<const unsigned char *>(str.c_str());
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
      char_array_3[i++] = *(bytes_to_encode++);
      if (i == 3)
      {
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (i = 0; (i < 4); i++)
          ret += chars[char_array_4[i]];
        i = 0;
      }
    }

    if (i)
    {
      for (j = i; j < 3; j++)
        char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

      for (j = 0; (j < i + 1); j++)
        ret += chars[char_array_4[j]];

      while ((i++ < 3))
        ret += '=';
    }

    return ret;
  }

  std::string base64::decode(std::string const &encoded_string)
  {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
      char_array_4[i++] = encoded_string[in_];
      in_++;
      if (i == 4)
      {
        for (i = 0; i < 4; i++)
          char_array_4[i] = chars.find(char_array_4[i]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (i = 0; (i < 3); i++)
          ret += char_array_3[i];
        i = 0;
      }
    }

    if (i)
    {
      for (j = 0; j < i; j++)
        char_array_4[j] = chars.find(char_array_4[j]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

      for (j = 0; (j < i - 1); j++)
        ret += char_array_3[j];
    }

    return ret;
  }

#endif //region_base64

} // namespace model
