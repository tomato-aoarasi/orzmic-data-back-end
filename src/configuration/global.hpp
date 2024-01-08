#pragma once 
#ifndef __CXX__GLOBAL_HPP
#define __CXX__GLOBAL_HPP

#include "sqlite_modern_cpp.h"

inline namespace self{
	namespace global {
		inline YAML::Node config_yaml{ YAML::LoadFile("config.yaml") };
	
		namespace server {
			inline std::string host{ config_yaml["server"]["host"].as<std::string>() };
			inline std::uint16_t port{ config_yaml["server"]["port"].as<std::uint16_t>() };
			inline uint32_t thread_num{ config_yaml["server"]["thread-num"].as<uint32_t>() };
			inline std::string authorization{ config_yaml["server"]["authorization"].as<std::string>() };
		}

		namespace db {
			inline sqlite::database orzmic{ sqlite::database(config_yaml["db"]["sqlite"]["orzmic"]["song"].as<std::string>())};
			inline sqlite::database orzmicRecord{ sqlite::database(config_yaml["db"]["sqlite"]["orzmic"]["record"].as<std::string>())};
			inline sqlite::database local{ sqlite::database(config_yaml["db"]["sqlite"]["local"].as<std::string>())};
		}
	}
}
#endif // !__CXX__GLOBAL_HPP
