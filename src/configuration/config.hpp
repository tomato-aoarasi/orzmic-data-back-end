/*
 * @File	  : config.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:14
 * @Introduce : 配置类(解析yaml)
*/

#pragma once

#ifndef __CXX20_CONFIG_HPP
#define __CXX20_CONFIG_HPP

#include <fstream>
#include <string>
#include <filesystem>
#include <limits>
#include <chrono>
#include "crow.h"
#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"
#include "configuration/define.hpp"
#include "configuration/global.hpp"
#include "common/log_system.hpp"

namespace Config {
	inline bool initializeGlobalVariables() {
		if (global::config_yaml["search-engine"]) {
			if (global::config_yaml["search-engine"]["meilisearch"]) {
				global::search::isOpen = true;
				global::search::meiliAuth = global::config_yaml["search-engine"]["meilisearch"]["authorization"].as<std::string>();
				global::search::meiliURL = global::config_yaml["search-engine"]["meilisearch"]["url"].as<std::string>();
			}
			if (global::config_yaml["search-engine"]["elasticsearch"]) {
				// Handle Elasticsearch
			}
		}
		return true;
	}
};

#include "common/utils.hpp"

#endif // !__CXX20_CONFIG_HPP