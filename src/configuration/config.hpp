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
	inline void initialized(void) {
	}
}

#include "common/utils.hpp"

#endif // !__CXX20_CONFIG_HPP