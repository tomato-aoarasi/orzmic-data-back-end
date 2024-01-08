#pragma once

#ifndef __CXX20_UTILS_HPP
#define __CXX20_UTILS_HPP
#include <functional>
#include <iostream>
#include <filesystem>
#include <random>
#include "common/self_exception.hpp"
#include "common/http_util.hpp"

#define __PRINT_FMT(function_name)  template <typename... Tn>inline void function_name(std::format_string<Tn...> fmt, Tn&&... args)

namespace self{
    template <typename T>
    [[maybe_unused]] inline void print(const T& arg) {
        std::cout << arg; std::cout.flush();
    }
    template <typename T, typename... Tn>
    [[maybe_unused]] inline void print(const T& arg, const Tn&... args) {
        if constexpr (sizeof...(args) > 0) {
            std::cout << arg << ' ';
        } print(args...);
    }
    template <typename T, typename... Tn>
    [[maybe_unused]] inline void println(const T& arg, const Tn&... args) {
        print(arg, args...); std::cout << std::endl;
    }
    template <typename T>
    [[maybe_unused]] inline void print(T&& arg) {
        std::cout << arg; std::cout.flush();
    }
    template <typename T, typename... Tn>
    [[maybe_unused]] inline void print(T&& arg, Tn&&... args) {
        if constexpr (sizeof...(args) > 0) {
            std::cout << arg << ' ';
        }
        print(args...);
    }
    template <typename T, typename... Tn>
    [[maybe_unused]] inline void println(T&& arg, Tn&&... args) {
        print(arg, args...); std::cout << std::endl;
    }

    __PRINT_FMT(print_fmt) {
        std::cout << std::format(fmt, std::forward<Tn>(args)...);
        std::cout.flush();
    }

    __PRINT_FMT(println_fmt) {
        print_fmt(fmt, std::forward<Tn>(args)...);
        std::cout << std::endl;
    }

    namespace common::utils {
        inline void createDirector(const std::string& logDirectoryPath) {
            namespace fs = std::filesystem;

            if (!fs::exists(logDirectoryPath)) {
                fs::create_directory(logDirectoryPath);
            }
        }

        inline std::string str_remove(const std::string& fullString, const std::string& substring) {
            std::size_t found = fullString.find(substring);
            if (found != std::string::npos) {
                return fullString.substr(found + substring.length());
            }
            else {
                throw std::runtime_error("Substring not found in the string.");
            }
        }

        // 运行shell脚本
        inline bool exec_simple(const char* cmd) {
            FILE* pipe{ popen(cmd, "r") };
            if (!pipe) {
                pclose(pipe);
                return false;
            }
            pclose(pipe);
            return true;
        }

        // 运行shell脚本并获取字符串
        inline const std::string exec(const char* cmd) {
            FILE* pipe = popen(cmd, "r");
            if (!pipe) {
                pclose(pipe);
                return "ERROR";
            }
            char buffer[128];
            std::string result = "";
            while (!feof(pipe)) {
                if (fgets(buffer, 128, pipe) != NULL)
                    result += buffer;
            }
            pclose(pipe);
            return result;
        }

        // 生成随机字符串
        inline std::string generateRandom(size_t length = 32, const std::string& charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
            std::random_device seeder; // 用于生成随机种子    mt19937 gen(rd()); // 以随机种子初始化随机数生成器

            const auto seed{ seeder.entropy() ? seeder() : time(nullptr) };

            std::mt19937 engine{ static_cast<std::mt19937::result_type>(seed) };

            std::uniform_int_distribution<size_t> distribution{ 0, charset.length() - 1 }; // 均匀分布

            std::string randomStr;
            for (size_t i = 0; i < length; i++) {
                randomStr += charset[distribution(engine)]; // 从字符集中随机选择一个字符    
            }
            return randomStr;
        }
    

        /// <summary>
        /// 判断vector<string>是否存在特定值
        /// </summary>
        /// <param name="keys">crow param的列表</param>
        /// <param name="val">是否含有特定字符串</param>
        /// <returns>true为存在,false为不存在</returns>
        inline bool hasParam(const std::vector<std::string>& keys, std::string_view val) {
            return std::find(keys.cbegin(), keys.cend(), val) != keys.cend();
        }

        /// <summary>
        /// 效验request参数
        /// </summary>
        /// <param name="req">req丢进去就完事了</param>
        /// <param name="val">是否存在的字符串</param>
        /// <returns>true返回长度不为0的内容</returns>
        inline bool verifyParam(const crow::request& req, std::string_view val) {
            bool has_value{ hasParam(req.url_params.keys(), val) };
            if (has_value)
            {
                return !std::string(req.url_params.get(val.data())).empty();
            }
            else return false;
        }


        // 字符串替换工具
        inline void replaceStrAll(std::string& str, const std::string& from, const std::string& to) {
            size_t pos = str.find(from);
            while (pos != std::string::npos) {
                str.replace(pos, from.length(), to);
                pos = str.find(from, pos + to.length());
            }
        };
    }

    inline crow::response HandleResponseBody(std::function<std::string(void)> f) {
        crow::response response;
        response.set_header("Content-Type", "application/json");
        try {
            response.write(f());
            response.code = 200;
            return response;
        }
        catch (const self::HTTPException& except) {
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(except.getCode(), except.getMessage(), except.getJson()).dump(2));
            response.code = except.getCode();
        }
        catch (const json::out_of_range& except) {
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(400, except.what()).dump(2));
            response.code = 400;
        }
        catch (const std::exception& except) {
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500, except.what()).dump(2));
            response.code = 500;
        }
        return response;
    }
}

#undef __PRINT_FMT
#endif // !__CXX20_UTILS_HPP