#ifndef GB28181_CONFIG_LOADER_H
#define GB28181_CONFIG_LOADER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace gb28181 {

class ConfigLoader {
public:
    static bool LoadJson(const std::string& filename, std::map<std::string, std::string>& config) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // 简化的JSON解析
        ParseJson(content, config);

        return true;
    }

private:
    static void ParseJson(const std::string& json, std::map<std::string, std::string>& config) {
        std::string key;
        std::string value;
        bool inKey = false;
        bool inValue = false;
        bool inString = false;

        for (size_t i = 0; i < json.length(); i++) {
            char c = json[i];

            if (c == '"' && json[i-1] != '\\') {
                inString = !inString;
                continue;
            }

            if (!inString) {
                if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':') {
                    if (!key.empty() && !value.empty()) {
                        // 去除空格和引号
                        key = Trim(key);
                        value = Trim(value);
                        if (!key.empty() && !value.empty()) {
                            config[key] = value;
                        }
                        key.clear();
                        value.clear();
                    }
                    continue;
                }
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    continue;
                }
            }

            if (c == ':') {
                inKey = false;
                inValue = true;
            } else if (c == '"' && json[i-1] != '\\') {
                // 已经处理
            } else if (c == '{') {
                // 进入嵌套对象
            } else if (c == '}') {
                // 退出嵌套对象
            } else {
                if (inKey) {
                    key += c;
                } else if (inValue) {
                    value += c;
                }
            }
        }

        // 处理最后一个键值对
        if (!key.empty() && !value.empty()) {
            key = Trim(key);
            value = Trim(value);
            if (!key.empty() && !value.empty()) {
                config[key] = value;
            }
        }
    }

    static std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r\"");
        size_t end = str.find_last_not_of(" \t\n\r\"");

        if (start == std::string::npos) {
            return "";
        }

        return str.substr(start, end - start + 1);
    }
};

} // namespace gb28181

#endif // GB28181_CONFIG_LOADER_H
