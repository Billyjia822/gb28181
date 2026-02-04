#include "utils/xml_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace gb28181 {

// XmlNode实现
XmlNode::XmlNode() {
}

XmlNode::~XmlNode() {
}

std::string XmlNode::GetAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    return (it != attributes_.end()) ? it->second : "";
}

void XmlNode::SetAttribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
}

void XmlNode::AddChild(std::shared_ptr<XmlNode> child) {
    children_.push_back(child);
}

std::shared_ptr<XmlNode> XmlNode::GetChild(const std::string& tagName) {
    for (auto& child : children_) {
        if (child->GetTagName() == tagName) {
            return child;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<XmlNode>> XmlNode::GetChildrenByTag(const std::string& tagName) {
    std::vector<std::shared_ptr<XmlNode>> result;
    for (auto& child : children_) {
        if (child->GetTagName() == tagName) {
            result.push_back(child);
        }
    }
    return result;
}

int XmlNode::GetIntValue() const {
    try {
        return std::stoi(text_);
    } catch (...) {
        return 0;
    }
}

double XmlNode::GetDoubleValue() const {
    try {
        return std::stod(text_);
    } catch (...) {
        return 0.0;
    }
}

// XmlParser实现
XmlParser::XmlParser() {
}

XmlParser::~XmlParser() {
}

std::shared_ptr<XmlNode> XmlParser::Parse(const std::string& xmlStr) {
    size_t pos = 0;
    SkipWhitespace(xmlStr, pos);

    // 跳过XML声明
    if (pos < xmlStr.length() && xmlStr[pos] == '<' && pos + 1 < xmlStr.length()) {
        if (xmlStr[pos + 1] == '?') {
            ParseDeclaration(xmlStr, pos);
            SkipWhitespace(xmlStr, pos);
        }
    }

    // 解析根节点
    return ParseElement(xmlStr, pos);
}

std::shared_ptr<XmlNode> XmlParser::ParseElement(const std::string& str, size_t& pos) {
    SkipWhitespace(str, pos);

    if (pos >= str.length() || str[pos] != '<') {
        return nullptr;
    }

    pos++; // 跳过'<'

    // 检查是否是结束标签
    if (pos < str.length() && str[pos] == '/') {
        return nullptr;
    }

    // 检查是否是注释
    if (pos + 3 < str.length() && str[pos] == '!' && str[pos + 1] == '-' && str[pos + 2] == '-') {
        ParseComment(str, pos);
        SkipWhitespace(str, pos);
        return ParseElement(str, pos);
    }

    // 解析标签名
    std::string tagName = ParseTagName(str, pos);
    auto node = std::make_shared<XmlNode>();
    node->SetTagName(tagName);

    // 解析属性
    SkipWhitespace(str, pos);
    while (pos < str.length() && str[pos] != '>' && str[pos] != '/') {
        auto attrs = ParseAttributes(str, pos);
        for (const auto& attr : attrs) {
            node->SetAttribute(attr.first, attr.second);
        }
        SkipWhitespace(str, pos);
    }

    // 检查是否是自闭合标签
    if (pos < str.length() && str[pos] == '/') {
        pos++; // 跳过'/'
        if (pos < str.length() && str[pos] == '>') {
            pos++; // 跳过'>'
        }
        return node;
    }

    // 跳过'>'
    if (pos < str.length() && str[pos] == '>') {
        pos++;
    }

    // 解析子节点和文本内容
    while (pos < str.length()) {
        SkipWhitespace(str, pos);

        if (pos >= str.length()) break;

        if (str[pos] == '<') {
            // 检查是否是结束标签
            if (pos + 1 < str.length() && str[pos + 1] == '/') {
                pos += 2; // 跳过'</'
                std::string endTag = ParseTagName(str, pos);
                SkipWhitespace(str, pos);
                if (pos < str.length() && str[pos] == '>') {
                    pos++;
                }
                break;
            } else {
                // 解析子节点
                auto child = ParseElement(str, pos);
                if (child) {
                    node->AddChild(child);
                }
            }
        } else {
            // 解析文本内容
            std::string text = ParseText(str, pos);
            node->SetText(text);
        }
    }

    return node;
}

void XmlParser::SkipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\t' ||
                                   str[pos] == '\n' || str[pos] == '\r')) {
        pos++;
    }
}

std::string XmlParser::ParseTagName(const std::string& str, size_t& pos) {
    std::string tagName;
    while (pos < str.length() && (isalnum(str[pos]) || str[pos] == '_' ||
                                   str[pos] == '-' || str[pos] == ':')) {
        tagName += str[pos];
        pos++;
    }
    return tagName;
}

std::map<std::string, std::string> XmlParser::ParseAttributes(const std::string& str, size_t& pos) {
    std::map<std::string, std::string> attrs;

    SkipWhitespace(str, pos);
    if (pos >= str.length() || str[pos] == '>' || str[pos] == '/') {
        return attrs;
    }

    // 解析属性名
    std::string attrName;
    while (pos < str.length() && (isalnum(str[pos]) || str[pos] == '_' ||
                                   str[pos] == '-')) {
        attrName += str[pos];
        pos++;
    }

    SkipWhitespace(str, pos);

    // 跳过'='
    if (pos < str.length() && str[pos] == '=') {
        pos++;
    }

    SkipWhitespace(str, pos);

    // 解析属性值（带引号）
    if (pos < str.length() && (str[pos] == '"' || str[pos] == '\'')) {
        char quote = str[pos];
        pos++; // 跳过引号

        std::string attrValue;
        while (pos < str.length() && str[pos] != quote) {
            // 处理转义字符
            if (str[pos] == '\\' && pos + 1 < str.length()) {
                pos++;
                attrValue += str[pos];
            } else {
                attrValue += str[pos];
            }
            pos++;
        }

        pos++; // 跳过结束引号
        attrs[attrName] = attrValue;
    }

    return attrs;
}

std::string XmlParser::ParseText(const std::string& str, size_t& pos) {
    std::string text;
    while (pos < str.length() && str[pos] != '<') {
        text += str[pos];
        pos++;
    }
    return text;
}

void XmlParser::ParseComment(const std::string& str, size_t& pos) {
    // 跳过<!-- -->
    pos += 3; // 跳过'<!-'
    while (pos + 2 < str.length()) {
        if (str[pos] == '-' && str[pos + 1] == '-' && str[pos + 2] == '>') {
            pos += 3;
            break;
        }
        pos++;
    }
}

void XmlParser::ParseDeclaration(const std::string& str, size_t& pos) {
    // 跳过<?xml ... ?>
    pos += 2; // 跳过'<?'
    while (pos + 1 < str.length()) {
        if (str[pos] == '?' && str[pos + 1] == '>') {
            pos += 2;
            break;
        }
        pos++;
    }
}

std::shared_ptr<XmlNode> XmlParser::CreateDocument(const std::string& rootTagName) {
    auto node = std::make_shared<XmlNode>();
    node->SetTagName(rootTagName);
    return node;
}

std::string XmlParser::ToString(std::shared_ptr<XmlNode> node, bool pretty) {
    if (!node) return "";

    std::stringstream ss;

    ss << "<" << node->GetTagName();

    // 添加属性
    for (const auto& attr : node->GetAttributes()) {
        ss << " " << attr.first << "=\"" << attr.second << "\"";
    }

    // 检查是否有子节点或文本内容
    if (node->GetChildren().empty() && node->GetText().empty()) {
        ss << "/>";
        return ss.str();
    }

    ss << ">";

    // 添加文本内容
    if (!node->GetText().empty()) {
        ss << node->GetText();
    }

    // 添加子节点
    for (const auto& child : node->GetChildren()) {
        if (pretty) {
            ss << "\n";
        }
        ss << ToString(child, pretty);
    }

    ss << "</" << node->GetTagName() << ">";

    return ss.str();
}

// MscdpXmlHelper实现
std::string MscdpXmlHelper::GetCommandType(const std::string& xmlStr) {
    size_t pos = xmlStr.find("<CmdType>");
    if (pos != std::string::npos) {
        pos += 10; // 跳过"<CmdType>"
        size_t endPos = xmlStr.find("</CmdType>", pos);
        if (endPos != std::string::npos) {
            return xmlStr.substr(pos, endPos - pos);
        }
    }
    return "";
}

std::string MscdpXmlHelper::GetDeviceId(const std::string& xmlStr) {
    size_t pos = xmlStr.find("<DeviceID>");
    if (pos != std::string::npos) {
        pos += 11; // 跳过"<DeviceID>"
        size_t endPos = xmlStr.find("</DeviceID>", pos);
        if (endPos != std::string::npos) {
            return xmlStr.substr(pos, endPos - pos);
        }
    }
    return "";
}

std::string MscdpXmlHelper::GetSN(const std::string& xmlStr) {
    size_t pos = xmlStr.find("<SN>");
    if (pos != std::string::npos) {
        pos += 4; // 跳过"<SN>"
        size_t endPos = xmlStr.find("</SN>", pos);
        if (endPos != std::string::npos) {
            return xmlStr.substr(pos, endPos - pos);
        }
    }
    return "";
}

bool MscdpXmlHelper::ParsePTZCommand(const std::string& xmlStr,
                                      int& command, int& speed, int& presetId) {
    command = 0;
    speed = 128;
    presetId = 0;

    // 查找PTZCmd节点
    size_t pos = xmlStr.find("<PTZCmd>");
    if (pos == std::string::npos) {
        return false;
    }

    pos += 8; // 跳过"<PTZCmd>"
    size_t endPos = xmlStr.find("</PTZCmd>", pos);
    if (endPos == std::string::npos) {
        return false;
    }

    std::string cmdStr = xmlStr.substr(pos, endPos - pos);

    // 解析Command
    pos = cmdStr.find("Command=");
    if (pos != std::string::npos) {
        pos += 8; // 跳过"Command="
        command = std::stoi(cmdStr.substr(pos));
    }

    // 解析Speed
    pos = cmdStr.find("Speed=");
    if (pos != std::string::npos) {
        pos += 6; // 跳过"Speed="
        speed = std::stoi(cmdStr.substr(pos));
    }

    // 解析PresetID
    pos = cmdStr.find("PresetID=");
    if (pos != std::string::npos) {
        pos += 9; // 跳过"PresetID="
        presetId = std::stoi(cmdStr.substr(pos));
    }

    return true;
}

bool MscdpXmlHelper::ParseRecordInfoQuery(const std::string& xmlStr,
                                           std::string& channelId,
                                           std::string& startTime,
                                           std::string& endTime) {
    channelId.clear();
    startTime.clear();
    endTime.clear();

    // 查找DeviceID
    size_t pos = xmlStr.find("<DeviceID>");
    if (pos != std::string::npos) {
        pos += 11;
        size_t endPos = xmlStr.find("</DeviceID>", pos);
        if (endPos != std::string::npos) {
            channelId = xmlStr.substr(pos, endPos - pos);
        }
    }

    // 查找StartTime
    pos = xmlStr.find("<StartTime>");
    if (pos != std::string::npos) {
        pos += 11;
        size_t endPos = xmlStr.find("</StartTime>", pos);
        if (endPos != std::string::npos) {
            startTime = xmlStr.substr(pos, endPos - pos);
        }
    }

    // 查找EndTime
    pos = xmlStr.find("<EndTime>");
    if (pos != std::string::npos) {
        pos += 9;
        size_t endPos = xmlStr.find("</EndTime>", pos);
        if (endPos != std::string::npos) {
            endTime = xmlStr.substr(pos, endPos - pos);
        }
    }

    return !channelId.empty();
}

} // namespace gb28181
