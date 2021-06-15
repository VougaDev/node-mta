

#include "main.hpp"

namespace boutch
{
	napi_value TRUE;
	napi_value FALSE;

	model::config configuration;
	smtp::sender sender;
	dkim::signer signer;

	napi_value send(napi_env env, napi_callback_info args)
	{
		size_t argc = 2;
		napi_value argv[2];
		napi_status status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);
		if (status != napi_ok)
			napi_throw_error(env, "BadArgumentException", "Failed to get args for email::send 1");

		model::email mail(configuration);

		node::get(env, argv[0], "from", mail.from);
		node::get(env, argv[0], "to", mail.to);
		node::get(env, argv[0], "list", mail.list);
		node::get(env, argv[0], "subject", mail.subject);
		node::get(env, argv[0], "html", mail.html);
		node::get(env, argv[0], "text", mail.text);
		node::get(env, argv[0], "replyTo", mail.replyTo);
		node::get(env, argv[0], "returnPath", mail.returnPath);
		mail.type = node::get(env, argv[0], "files", mail.files);

		mail.prepare();

		std::vector<model::header> *headers = &mail.headers;
		headers->emplace_back("MIME-Version", "1.0", true);
		headers->emplace_back("From", mail.from.toString(), true);
		//headers->emplace_back("To", mail.to[0].toString(), true); Will be updated before signing the message
		if(!mail.list.address.empty()){
			headers->emplace_back("To", mail.list.toString(),true);
		}
		headers->emplace_back("Subject", mail.subject, true);
		headers->emplace_back("Date", mail.date, true);
		headers->emplace_back("Message-ID", mail.id, true);
	

		if(!mail.replyTo.empty()){
			headers->emplace_back("Reply-To", mail.replyTo, false);
		}
		if(!mail.returnPath.empty()){
			headers->emplace_back("Return-Path", mail.returnPath, true);
		}

		
		//if (true){ std::cout << mail << std::endl; return FALSE;}

		napi_value exchange;
		uint32_t length = 0;
		status = napi_get_array_length(env, argv[1], &length);
		if (status != napi_ok)
		{
			return FALSE;
		}
		if (length < 1)
			return FALSE;
		std::string mx;
		for (uint32_t i = 0; i < length; i++)
		{
			status = napi_get_element(env, argv[1], i, &exchange);
			mx.clear();
			node::get(env, exchange, "exchange", mx);

			std::cout << "Mx Host: " << mx << std::endl;
			if(sender.send(mail,mx)) {
				std::cout << "Email sent !!"<<std::endl;
				return TRUE;
			}
			std::cout << "Email not sent to "<< mx << "- Attempt "<<i<<"/"<<length<<std::endl;
			sleep(10);
		}
		return FALSE;
	}

	napi_value configure(napi_env env, napi_callback_info args)
	{
		size_t argc = 2;
		napi_value argv[2];
		napi_status status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);
		if (status != napi_ok)
			return NULL;

		node::get(env, argv[0], "hostname", configuration.hostname);
		node::get(env, argv[0], "privateKey", configuration.privateKey);
		node::get(env, argv[0], "port", configuration.port);
		node::get(env, argv[0], "dkimSelector", configuration.dkimSelector);
		node::get(env, argv[0], "publicKey", configuration.publicKey);
		signer = dkim::signer(
				configuration,
				dkim::CanonicalizationType::RELAXED(),
				dkim::CanonicalizationType::SIMPLE(),
				dkim::SigningAlgorithmType::RSA_SHA256(configuration));
		sender = smtp::sender(configuration,signer);
		std::string result(configuration.hostname);
#ifdef DEBUG
		result.append("\n").append(configuration.privateKey).append("\n").append(std::to_string(configuration.port));
#endif

		napi_value res;
		status = napi_create_string_utf8(
				env,
				result.c_str(),
				result.length(),
				&res);
		if (status != napi_ok)
		{
			napi_throw_error(env, "What the f***", "Failed to do it");
		}
		return res;
	}

	napi_value hello(napi_env env, napi_callback_info args)
	{
		napi_value greeting;
		napi_status status;

		status = napi_create_string_utf8(env, "Node MTA/1.0.0", NAPI_AUTO_LENGTH, &greeting);
		if (status != napi_ok)
			return nullptr;
		return greeting;
	}

	napi_value init(napi_env env, napi_value exports)
	{
		napi_status status;
		napi_value fn, conf, snd;

		status = napi_create_function(env, nullptr, 0, hello, nullptr, &fn);
		if (status != napi_ok)
			return nullptr;
		status = napi_create_function(env, nullptr, 0, send, nullptr, &snd);
		if (status != napi_ok)
			return nullptr;
		status = napi_create_function(env, nullptr, 0, configure, nullptr, &conf);
		if (status != napi_ok)
			return nullptr;

		status = napi_set_named_property(env, exports, "name", fn);
		if (status != napi_ok)
			return nullptr;

		status = napi_set_named_property(env, exports, "configure", conf);
		if (status != napi_ok)
			return nullptr;

		status = napi_set_named_property(env, exports, "send", snd);
		if (status != napi_ok)
			return nullptr;

		napi_create_int32(env, 1, &TRUE);
		napi_create_int32(env, 0, &FALSE);
		return exports;
	}

	NAPI_MODULE(NODE_GYP_MODULE_NAME, init)

	bool node::get(napi_env &env, napi_value &obj, const char *key, std::string &value)
	{
		bool hasProp;
		napi_has_named_property(env, obj, key, &hasProp);
		if (hasProp)
		{
			size_t length = 0;
			napi_value n_key, n_value;
			napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &n_key);
			napi_get_property(env, obj, n_key, &n_value);
			char hn_buff[4096];
			napi_status status = napi_get_value_string_utf8(env, n_value, hn_buff, 8192, &length);
			if (status == napi_ok)
			{
				value.reserve(length);
				value.append(hn_buff);
				value.shrink_to_fit();
				return true;
			}
		}
		return false;
	}

	bool node::get(napi_env &env, napi_value &obj, const char *key, size_t &value)
	{
		bool hasProp;
		napi_has_named_property(env, obj, key, &hasProp);
		if (hasProp)
		{
			napi_value n_key, n_value;
			napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &n_key);
			napi_get_property(env, obj, n_key, &n_value);
			int32_t res;
			napi_status status = napi_get_value_int32(env, n_value, &res);
			if (status == napi_ok)
			{
				value = (size_t)res;
				return true;
			}
		}
		return false;
	}

	bool node::get(napi_env &env, napi_value &obj, const char *key, bool &value)
	{
		bool hasProp;
		napi_has_named_property(env, obj, key, &hasProp);
		if (hasProp)
		{
			napi_value n_key, n_value;
			napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &n_key);
			napi_get_property(env, obj, n_key, &n_value);
			napi_status status = napi_get_value_bool(env, n_value, &value);
			if (status == napi_ok)
				return true;
		}
		return false;
	}
	
	void node::get(napi_env &env, napi_value &obj, const char *key, std::vector<model::address> &values){
		bool hasProp;
		napi_has_named_property(env, obj, key, &hasProp);
		if (hasProp){

			uint32_t len;
			napi_value arr;
			napi_get_named_property(env,obj,key,&arr);
			
			napi_is_array(env,arr,&hasProp);
			if(hasProp){
				napi_get_array_length(env,arr,&len);
				if(len>0){
					napi_value value;
					for(uint32_t i = 0; i < len;i++){
						model::address addr;
						napi_get_element(env,arr,i,&value);
						node::get(env,value,"name",addr.name);
						node::get(env,value,"address",addr.address);
						values.push_back(addr);
					}	
				}
			}else{
				model::address addr;
				node::get(env,arr,"name",addr.name);
				node::get(env,arr,"address",addr.address);
				values.push_back(addr);
			}
		}
	}

  void node::get(napi_env &env, napi_value &obj, const char *key, model::address &value)
	{
		bool hasProp;
		napi_has_named_property(env, obj, key, &hasProp);
		if (hasProp)
		{
			napi_value n_key, n_value;
			napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &n_key);
			napi_get_property(env, obj, n_key, &n_value);

			napi_has_named_property(env, n_value, "name", &hasProp);
			if (hasProp)
			{
				node::get(env, n_value, "name", value.name);
			}
			napi_has_named_property(env, n_value, "address", &hasProp);
			if (hasProp)
			{
				node::get(env, n_value, "address", value.address);
			}
			else
			{
				napi_throw_error(env,
												 "BadArgumentException", "In order to send email you must "
																								 "give me a sender and receiver address - For example: name@example.com");
			}
		}
	}

	model::FileType node::get(napi_env &env, napi_value &obj, const char *key, std::vector<model::file> &value)
	{
		napi_value n_key, n_value, sub;
		std::string id, path, name;
		bool inLine;
		model::FileType type = model::FileType::NONE;
		napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &n_key);
		napi_has_named_property(env, obj, key, &inLine);
		if (inLine)
		{

			napi_get_property(env, obj, n_key, &n_value);
			uint32_t length;
			napi_get_array_length(env, n_value, &length);
			if (length > 0)
			{
				for (uint32_t i = 0; i < length; i++)
				{
					id.clear();
					path.clear();
					name.clear();
					napi_get_element(env, n_value, i, &sub);

					napi_has_named_property(env, sub, "id", &inLine);
					if (inLine)
					{
						node::get(env, sub, "id", id);
					}
					napi_has_named_property(env, sub, "path", &inLine);
					if (inLine)
					{
						node::get(env, sub, "path", path);
					}
					napi_has_named_property(env, sub, "name", &inLine);
					if (inLine)
					{
						node::get(env, sub, "name", name);
					}
					napi_has_named_property(env, sub, "inline", &inLine);
					if (inLine)
					{
						node::get(env, sub, "inline", inLine);
						if (inLine)
						{
							if (type == model::FileType::NONE)
								type = model::FileType::INLINE;
							else if (type == model::FileType::ATTACHMENT)
								type = model::FileType::BOTH;
						}
						else
						{
							if (type == model::FileType::NONE)
								type = model::FileType::ATTACHMENT;
							else if (type == model::FileType::INLINE)
								type = model::FileType::BOTH;
						}
					}
					value.emplace_back(id, path, name, inLine);
				}
			}
		}
		return type;
	}

} // namespace boutch
