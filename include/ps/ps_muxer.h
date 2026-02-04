#ifndef GB28181_PS_MUXER_H
#define GB28181_PS_MUXER_H

#include <string>
#include <vector>
#include <memory>

namespace gb28181 {

enum class StreamType {
    H264,
    H265,
    AAC,
    G711A,
    G711U
};

struct NaluUnit {
    uint8_t type;
    std::vector<uint8_t> data;
};

class PsMuxer {
public:
    PsMuxer();
    ~PsMuxer();

    // 初始化PS封装器
    bool Initialize(StreamType videoType, StreamType audioType);

    // 写入H.264 NALU
    bool WriteH264Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts);

    // 写入H.265 NALU
    bool WriteH265Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts);

    // 写入音频数据
    bool WriteAudioData(const uint8_t* data, size_t len, uint64_t pts);

    // 获取封装后的PS数据
    std::vector<uint8_t> GetPsData();

    // 清空缓冲区
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gb28181

#endif // GB28181_PS_MUXER_H
