#include "utils/md5.h"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace gb28181 {

// MD5常量
static const uint8_t PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F((b), (c), (d)) + (x) + (ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
    (a) += G((b), (c), (d)) + (x) + (ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
    (a) += H((b), (c), (d)) + (x) + (ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
    (a) += I((b), (c), (d)) + (x) + (ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

MD5::MD5() : finalized_(false) {
    memset(buffer_, 0, sizeof(buffer_));
    state_[0] = 0x67452301;
    state_[1] = 0xEFCDAB89;
    state_[2] = 0x98BADCFE;
    state_[3] = 0x10325476;
    count_[0] = 0;
    count_[1] = 0;
    digest_.resize(16);
}

MD5::~MD5() {
}

void MD5::Update(const uint8_t* data, size_t len) {
    if (finalized_) {
        return;
    }

    uint32_t i, index, partLen;

    index = (uint32_t)((count_[0] >> 3) & 0x3F);

    if ((count_[0] += ((uint32_t)len << 3)) < ((uint32_t)len << 3)) {
        count_[1]++;
    }
    count_[1] += ((uint32_t)len >> 29);

    partLen = 64 - index;

    if (len >= partLen) {
        memcpy(&buffer_[index], data, partLen);
        Transform(buffer_);

        for (i = partLen; i + 63 < len; i += 64) {
            Transform(&data[i]);
        }
        index = 0;
    } else {
        i = 0;
    }

    memcpy(&buffer_[index], &data[i], len - i);
}

void MD5::Update(const std::string& str) {
    Update(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

std::vector<uint8_t> MD5::Final() {
    if (finalized_) {
        return digest_;
    }

    uint8_t bits[8];
    uint32_t index, padLen;

    Encode(bits, count_, 8);

    index = (uint32_t)((count_[0] >> 3) & 0x3F);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    Update(PADDING, padLen);
    Update(bits, 8);

    Encode(&digest_[0], state_, 16);

    memset(buffer_, 0, sizeof(buffer_));
    memset(state_, 0, sizeof(state_));
    memset(count_, 0, sizeof(count_));
    finalized_ = true;

    return digest_;
}

std::string MD5::GetHexString() const {
    if (!finalized_) {
        return "";
    }

    std::stringstream ss;
    for (size_t i = 0; i < digest_.size(); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest_[i];
    }
    return ss.str();
}

std::string MD5::Digest(const std::string& str) {
    MD5 md5;
    md5.Update(str);
    md5.Final();
    return md5.GetHexString();
}

std::string MD5::CalculateDigestResponse(
    const std::string& method,
    const std::string& uri,
    const std::string& username,
    const std::string& realm,
    const std::string& password,
    const std::string& nonce,
    const std::string& qop,
    const std::string& cseq) {

    // 计算HA1 = MD5(username:realm:password)
    std::string ha1Input = username + ":" + realm + ":" + password;
    std::string ha1 = MD5::Digest(ha1Input);

    // 计算HA2
    std::string ha2Input;
    if (qop == "auth-int") {
        // auth-int模式（需要实体体，暂不支持）
        ha2Input = method + ":" + uri;
    } else {
        // auth模式
        ha2Input = method + ":" + uri;
    }
    std::string ha2 = MD5::Digest(ha2Input);

    // 计算response
    std::string responseInput;
    if (qop.empty() || qop == "auth") {
        if (!qop.empty()) {
            // 有QOP: response = MD5(HA1:nonce:nc:cnonce:qop:HA2)
            // 这里简化处理，使用默认值
            std::string nc = "00000001";
            std::string cnonce = "0a4f113b";
            responseInput = ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2;
        } else {
            // 无QOP: response = MD5(HA1:nonce:HA2)
            responseInput = ha1 + ":" + nonce + ":" + ha2;
        }
    }

    return MD5::Digest(responseInput);
}

void MD5::Transform(const uint8_t block[64]) {
    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t x[16];

    Decode(x, block, 64);

    FF(a, b, c, d, x[0], 7, 0xD76AA478);
    FF(d, a, b, c, x[1], 12, 0xE8C7B756);
    FF(c, d, a, b, x[2], 17, 0x242070DB);
    FF(b, c, d, a, x[3], 22, 0xC1BDCEEE);
    FF(a, b, c, d, x[4], 7, 0xF57C0FAF);
    FF(d, a, b, c, x[5], 12, 0x4787C62A);
    FF(c, d, a, b, x[6], 17, 0xA8304613);
    FF(b, c, d, a, x[7], 22, 0xFD469501);
    FF(a, b, c, d, x[8], 7, 0x698098D8);
    FF(d, a, b, c, x[9], 12, 0x8B44F7AF);
    FF(c, d, a, b, x[10], 17, 0xFFFF5BB1);
    FF(b, c, d, a, x[11], 22, 0x895CD7BE);
    FF(a, b, c, d, x[12], 7, 0x6B901122);
    FF(d, a, b, c, x[13], 12, 0xFD987193);
    FF(c, d, a, b, x[14], 17, 0xA679438E);
    FF(b, c, d, a, x[15], 22, 0x49B40821);

    GG(a, b, c, d, x[1], 5, 0xF61E2562);
    GG(d, a, b, c, x[6], 9, 0xC040B340);
    GG(c, d, a, b, x[11], 14, 0x265E5A51);
    GG(b, c, d, a, x[0], 20, 0xE9B6C7AA);
    GG(a, b, c, d, x[5], 5, 0xD62F105D);
    GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xD8A1E681);
    GG(b, c, d, a, x[4], 20, 0xE7D3FBC8);
    GG(a, b, c, d, x[9], 5, 0x21E1CDE6);
    GG(d, a, b, c, x[14], 9, 0xC33707D6);
    GG(c, d, a, b, x[3], 14, 0xF4D50D87);
    GG(b, c, d, a, x[8], 20, 0x455A14ED);
    GG(a, b, c, d, x[13], 5, 0xA9E3E905);
    GG(d, a, b, c, x[2], 9, 0xFCEFA3F8);
    GG(c, d, a, b, x[7], 14, 0x676F02D9);
    GG(b, c, d, a, x[12], 20, 0x8D2A4C8A);

    HH(a, b, c, d, x[5], 4, 0xFFFA3942);
    HH(d, a, b, c, x[8], 11, 0x8771F681);
    HH(c, d, a, b, x[11], 16, 0x6D9D6122);
    HH(b, c, d, a, x[14], 23, 0xFDE5380C);
    HH(a, b, c, d, x[1], 4, 0xA4BEEA44);
    HH(d, a, b, c, x[4], 11, 0x4BDECFA9);
    HH(c, d, a, b, x[7], 16, 0xF6BB4B60);
    HH(b, c, d, a, x[10], 23, 0xBEBFBC70);
    HH(a, b, c, d, x[13], 4, 0x289B7EC6);
    HH(d, a, b, c, x[0], 11, 0xEAA127FA);
    HH(c, d, a, b, x[3], 16, 0xD4EF3085);
    HH(b, c, d, a, x[6], 23, 0x04881D05);
    HH(a, b, c, d, x[9], 4, 0xD9D4D039);
    HH(d, a, b, c, x[12], 11, 0xE6DB99E5);
    HH(c, d, a, b, x[15], 16, 0x1FA27CF8);
    HH(b, c, d, a, x[2], 23, 0xC4AC5665);

    II(a, b, c, d, x[0], 6, 0xF4292244);
    II(d, a, b, c, x[7], 10, 0x432AFF97);
    II(c, d, a, b, x[14], 15, 0xAB9423A7);
    II(b, c, d, a, x[5], 21, 0xFC93A039);
    II(a, b, c, d, x[12], 6, 0x655B59C3);
    II(d, a, b, c, x[3], 10, 0x8F0CCC92);
    II(c, d, a, b, x[10], 15, 0xFFEFF47D);
    II(b, c, d, a, x[1], 21, 0x85845DD1);
    II(a, b, c, d, x[8], 6, 0x6FA87E4F);
    II(d, a, b, c, x[15], 10, 0xFE2CE6E0);
    II(c, d, a, b, x[6], 15, 0xA3014314);
    II(b, c, d, a, x[13], 21, 0x4E0811A1);
    II(a, b, c, d, x[4], 6, 0xF7537E82);
    II(d, a, b, c, x[11], 10, 0xBD3AF235);
    II(c, d, a, b, x[2], 15, 0x2AD7D2BB);
    II(b, c, d, a, x[9], 21, 0xEB86D391);

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;

    memset(x, 0, sizeof(x));
}

void MD5::Encode(uint8_t* output, const uint32_t* input, size_t len) {
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uint8_t)(input[i] & 0xFF);
        output[j + 1] = (uint8_t)((input[i] >> 8) & 0xFF);
        output[j + 2] = (uint8_t)((input[i] >> 16) & 0xFF);
        output[j + 3] = (uint8_t)((input[i] >> 24) & 0xFF);
    }
}

void MD5::Decode(uint32_t* output, const uint8_t* input, size_t len) {
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
        output[i] = ((uint32_t)input[j]) |
                   (((uint32_t)input[j + 1]) << 8) |
                   (((uint32_t)input[j + 2]) << 16) |
                   (((uint32_t)input[j + 3]) << 24);
    }
}

} // namespace gb28181
