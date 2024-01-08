#pragma once

#ifndef __CXX20_DEFINE_HPP
#define __CXX20_DEFINE_HPP

#include <string>
#include <chrono>
#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include "crow.h"

#define elif else if

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace std {
    using fmt::format;
    using fmt::format_string;
    using fmt::format_error;
    using fmt::formatter;
}

using CrowApplication = crow::SimpleApp;
using nlohmann::json;

namespace define {
	struct AccessControl {
		unsigned int sid{ std::numeric_limits<unsigned int>::max() };
		std::string account{ "" };
		unsigned int hits{ 0 };
		std::string token{ "" };
		unsigned char authority{ 0 };
	};

	struct OrzmicLevel {
		std::unique_ptr<std::string> design{ nullptr };
		std::unique_ptr<std::string> difficulty{ nullptr };
		std::unique_ptr<uint32_t> note{ nullptr };
		std::unique_ptr<float> rating{ nullptr };
	};
};

#endif // !__CXX20_DEFINE_HPP