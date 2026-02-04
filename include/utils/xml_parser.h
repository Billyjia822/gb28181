#ifndef GB28181_XML_PARSER_H
#define GB28181_XML_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace gb28181 {

/**
 * @brief XML元素节点
 */
class XmlNode {
public:
    XmlNode();
    ~XmlNode();

    /**
     * @brief 获取标签名
     */
    std::string GetTagName() const { return tagName_; }

    /**
     * @brief 设置标签名
     */
    void SetTagName(const std::string& name) { tagName_ = name; }

    /**
     * @brief 获取文本内容
     */
    std::string GetText() const { return text_; }

    /**
     * @brief 设置文本内容
     */
    void SetText(const std::string& text) { text_ = text; }

    /**
     * @brief 获取属性值
     */
    std::string GetAttribute(const std::string& name) const;

    /**
     * @brief 设置属性值
     */
    void SetAttribute(const std::string& name, const std::string& value);

    /**
     * @brief 获取所有属性
     */
    std::map<std::string, std::string> GetAttributes() const { return attributes_; }

    /**
     * @brief 添加子节点
     */
    void AddChild(std::shared_ptr<XmlNode> child);

    /**
     * @brief 获取子节点
     */
    std::shared_ptr<XmlNode> GetChild(const std::string& tagName);

    /**
     * @brief 获取所有子节点
     */
    std::vector<std::shared_ptr<XmlNode>> GetChildren() const { return children_; }

    /**
     * @brief 获取指定标签名的所有子节点
     */
    std::vector<std::shared_ptr<XmlNode>> GetChildrenByTag(const std::string& tagName);

    /**
     * @brief 获取整数值
     */
    int GetIntValue() const;

    /**
     * @brief 获取浮点数值
     */
    double GetDoubleValue() const;

private:
    std::string tagName_;
    std::string text_;
    std::map<std::string, std::string> attributes_;
    std::vector<std::shared_ptr<XmlNode>> children_;
};

/**
 * @brief XML解析器
 * 用于解析GB28181 MANSCDP协议的XML消息
 */
class XmlParser {
public:
    XmlParser();
    ~XmlParser();

    /**
     * @brief 解析XML字符串
     * @param xmlStr XML字符串
     * @return XML根节点
     */
    std::shared_ptr<XmlNode> Parse(const std::string& xmlStr);

    /**
     * @brief 创建XML文档
     * @param rootTagName 根节点标签名
     * @return XML根节点
     */
    std::shared_ptr<XmlNode> CreateDocument(const std::string& rootTagName);

    /**
     * @brief 将XML节点转换为字符串
     * @param node XML节点
     * @param pretty 是否格式化输出
     * @return XML字符串
     */
    std::string ToString(std::shared_ptr<XmlNode> node, bool pretty = false);

private:
    /**
     * @brief 跳过空白字符
     */
    void SkipWhitespace(const std::string& str, size_t& pos);

    /**
     * @brief 解析标签名
     */
    std::string ParseTagName(const std::string& str, size_t& pos);

    /**
     * @brief 解析属性
     */
    std::map<std::string, std::string> ParseAttributes(const std::string& str, size_t& pos);

    /**
     * @brief 解析文本内容
     */
    std::string ParseText(const std::string& str, size_t& pos);

    /**
     * @brief 解析注释
     */
    void ParseComment(const std::string& str, size_t& pos);

    /**
     * @brief 解析XML声明
     */
    void ParseDeclaration(const std::string& str, size_t& pos);

    /**
     * @brief 解析XML元素
     */
    std::shared_ptr<XmlNode> ParseElement(const std::string& str, size_t& pos);
};

/**
 * @brief MANSCDP协议XML解析辅助类
 */
class MscdpXmlHelper {
public:
    /**
     * @brief 从XML字符串中提取命令类型
     */
    static std::string GetCommandType(const std::string& xmlStr);

    /**
     * @brief 从XML字符串中提取设备ID
     */
    static std::string GetDeviceId(const std::string& xmlStr);

    /**
     * @brief 从XML字符串中提取序列号
     */
    static std::string GetSN(const std::string& xmlStr);

    /**
     * @brief 解析PTZ命令
     */
    static bool ParsePTZCommand(const std::string& xmlStr,
                                int& command, int& speed, int& presetId);

    /**
     * @brief 解析录像查询参数
     */
    static bool ParseRecordInfoQuery(const std::string& xmlStr,
                                     std::string& channelId,
                                     std::string& startTime,
                                     std::string& endTime);
};

} // namespace gb28181

#endif // GB28181_XML_PARSER_H
