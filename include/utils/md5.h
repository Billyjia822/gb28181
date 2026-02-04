#ifndef GB28181_MD5_H
#define GB28181_MD5_H

#include <string>
#include <vector>
#include <cstdint>

namespace gb28181 {

/**
 * @brief MD5哈希算法工具类
 * 用于SIP Digest认证计算
 */
class MD5 {
public:
    MD5();
    ~MD5();

    /**
     * @brief 更新MD5计算
     * @param data 输入数据
     * @param len 数据长度
     */
    void Update(const uint8_t* data, size_t len);

    /**
     * @brief 更新MD5计算（字符串）
     * @param str 输入字符串
     */
    void Update(const std::string& str);

    /**
     * @brief 完成MD5计算并获取结果
     * @return 16字节的MD5哈希值
     */
    std::vector<uint8_t> Final();

    /**
     * @brief 获取MD5的十六进制字符串表示
     * @return 32字符的十六进制字符串
     */
    std::string GetHexString() const;

    /**
     * @brief 计算字符串的MD5
     * @param str 输入字符串
     * @return MD5十六进制字符串
     */
    static std::string Digest(const std::string& str);

    /**
     * @brief 计算SIP Digest认证响应
     * @param method SIP方法（REGISTER, INVITE等）
     * @param uri 请求URI
     * @param username 用户名
     * @param realm 认证域
     * @param password 密码
     * @param nonce 服务器提供的nonce
     * @param qop 保护质量（可选）
     * @param cseq CSeq值
     * @return Digest响应值
     */
    static std::string CalculateDigestResponse(
        const std::string& method,
        const std::string& uri,
        const std::string& username,
        const std::string& realm,
        const std::string& password,
        const std::string& nonce,
        const std::string& qop = "",
        const std::string& cseq = ""
    );

private:
    void Transform(const uint8_t block[64]);
    void Encode(uint8_t* output, const uint32_t* input, size_t len);
    void Decode(uint32_t* output, const uint8_t* input, size_t len);

    uint8_t buffer_[64];
    uint32_t state_[4];
    uint32_t count_[2];
    bool finalized_;
    std::vector<uint8_t> digest_;
};

} // namespace gb28181

#endif // GB28181_MD5_H
