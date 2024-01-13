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

        // ����shell�ű�
        inline bool exec_simple(const char* cmd) {
            FILE* pipe{ popen(cmd, "r") };
            if (!pipe) {
                pclose(pipe);
                return false;
            }
            pclose(pipe);
            return true;
        }

        // ����shell�ű�����ȡ�ַ���
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

        // ��������ַ���
        inline std::string generateRandom(size_t length = 32, const std::string& charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
            std::random_device seeder; // ���������������    mt19937 gen(rd()); // ��������ӳ�ʼ�������������

            const auto seed{ seeder.entropy() ? seeder() : time(nullptr) };

            std::mt19937 engine{ static_cast<std::mt19937::result_type>(seed) };

            std::uniform_int_distribution<size_t> distribution{ 0, charset.length() - 1 }; // ���ȷֲ�

            std::string randomStr;
            for (size_t i = 0; i < length; i++) {
                randomStr += charset[distribution(engine)]; // ���ַ��������ѡ��һ���ַ�    
            }
            return randomStr;
        }
    

        /// <summary>
        /// �ж�vector<string>�Ƿ�����ض�ֵ
        /// </summary>
        /// <param name="keys">crow param���б�</param>
        /// <param name="val">�Ƿ����ض��ַ���</param>
        /// <returns>trueΪ����,falseΪ������</returns>
        inline bool hasParam(const std::vector<std::string>& keys, std::string_view val) {
            return std::find(keys.cbegin(), keys.cend(), val) != keys.cend();
        }

        /// <summary>
        /// Ч��request����
        /// </summary>
        /// <param name="req">req����ȥ��������</param>
        /// <param name="val">�Ƿ���ڵ��ַ���</param>
        /// <returns>true���س��Ȳ�Ϊ0������</returns>
        inline bool verifyParam(const crow::request& req, std::string_view val) {
            bool has_value{ hasParam(req.url_params.keys(), val) };
            if (has_value)
            {
                return !std::string(req.url_params.get(val.data())).empty();
            }
            else return false;
        }

        // base64����Ĺ���
        inline static std::vector<std::uint8_t> base64Decode(const std::string& base64Str) {
            static const std::string base64Chars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789+/";

            std::vector<std::uint8_t> data;
            size_t i = 0;
            uint32_t n = 0;
            int padding = 0;

            while (i < base64Str.length()) {
                char c = base64Str[i++];
                if (c == '=') {
                    padding++;
                    continue;
                }
                size_t index = base64Chars.find(c);
                if (index == std::string::npos) {
                    continue;
                }
                n = (n << 6) | index;
                if (i % 4 == 0) {
                    data.push_back((n >> 16) & 0xFF);
                    data.push_back((n >> 8) & 0xFF);
                    data.push_back(n & 0xFF);
                    n = 0;
                }
            }
            if (padding > 0) {
                n <<= padding * 6;
                data.push_back((n >> 16) & 0xFF);
                if (padding == 1) {
                    data.push_back((n >> 8) & 0xFF);
                }
            }
            return data;
        }

        // �ַ����滻����
        inline void replaceStrAll(std::string& str, const std::string& from, const std::string& to) {
            size_t pos = str.find(from);
            while (pos != std::string::npos) {
                str.replace(pos, from.length(), to);
                pos = str.find(from, pos + to.length());
            }
        };

        // С���Ƚ� 0����,1���ڣ�-1С��, accuracy�Ǿ���
        template <typename T = const double>
        constexpr inline std::int8_t compareDecimal(T x1, T x2, T accuracy = 1e-5) {
            constexpr std::int8_t BIG{ 1 }, SMALL{ -1 }, EQUAL{ 0 };
            T diff{ std::abs(x1 - x2) };
            if (diff <= accuracy) {
                return EQUAL;
            } else if (x1 > x2) {
                return BIG;
            } else {
                return SMALL;
            }
            return 0;
        }
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