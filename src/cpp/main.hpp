#ifndef CPP_MAIL_MAIN_H
#define CPP_MAIL_MAIN_H

#include <node_api.h>
#include "smtp.hpp"

namespace boutch
{
  	struct node
	{
		static bool get(napi_env &env, napi_value &obj, const char *key, std::string &value);
		static bool get(napi_env &env, napi_value &obj, const char *key, size_t &value);
		static bool get(napi_env &env, napi_value &obj, const char *key, bool &value);
		static void get(napi_env &env, napi_value &obj, const char *key, model::address &value);
		static void get(napi_env &env, napi_value &obj, const char *key, std::vector<model::address> &value);
		static model::FileType get(napi_env &env, napi_value &obj, const char *key, std::vector<model::file> &value);
	};

} // namespace boutch

#endif //CPP_MAIL_MAIN_H