#pragma once 
#ifndef __CXX__GLOBAL_HPP
#define __CXX__GLOBAL_HPP

#include "sqlite_modern_cpp.h"

inline namespace self{
	namespace global {
		inline const YAML::Node config_yaml{ YAML::LoadFile("config.yaml") };
	
		namespace server {
			inline const std::string host{ config_yaml["server"]["host"].as<std::string>() };
			inline const std::uint16_t port{ config_yaml["server"]["port"].as<std::uint16_t>() };
			inline const uint32_t thread_num{ config_yaml["server"]["thread-num"].as<uint32_t>() };
			inline const std::string authorization{ config_yaml["server"]["authorization"].as<std::string>() };
		}

		namespace db {
			inline sqlite::database orzmic{ sqlite::database(config_yaml["db"]["sqlite"]["orzmic"]["song"].as<std::string>())};
			inline sqlite::database orzmicRecord{ sqlite::database(config_yaml["db"]["sqlite"]["orzmic"]["record"].as<std::string>())};
			inline sqlite::database local{ sqlite::database(config_yaml["db"]["sqlite"]["local"].as<std::string>())};
		}

		namespace search {
			inline bool isOpen{ false };
			inline const std::string orzmicSearchNamespace{ "orzmic" };
			inline std::string meiliURL{ };
			inline std::string meiliAuth{ };
		}
	}
}
#endif // !__CXX__GLOBAL_HPP
