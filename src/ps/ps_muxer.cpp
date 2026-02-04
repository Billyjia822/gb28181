#include "ps/ps_muxer.h"
#include <iostream>

namespace gb28181 {

class PsMuxer::Impl {
public:
    Impl() : initialized_(false) {}

    bool Initialize(StreamType videoType, StreamType audioType) {
        videoType_ = videoType;
        audioType_ = audioType;
        initialized_ = true;
        return true;
    }

    bool WriteH264Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts) {
        if (!initialized_) {
            return false;
        }
        // TODO: 实现H.264 NALU封装
        return true;
    }

    bool WriteH265Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts) {
        if (!initialized_) {
            return false;
        }
        // TODO: 实现H.265 NALU封装
        return true;
    }

    bool WriteAudioData(const uint8_t* data, size_t len, uint64_t pts) {
        if (!initialized_) {
            return false;
        }
        // TODO: 实现音频数据封装
        return true;
    }

    std::vector<uint8_t> GetPsData() {
        // TODO: 返回封装后的PS数据
        return psBuffer_;
    }

    void Clear() {
        psBuffer_.clear();
    }

private:
    bool initialized_;
    StreamType videoType_;
    StreamType audioType_;
    std::vector<uint8_t> psBuffer_;
};

PsMuxer::PsMuxer() : impl_(new Impl()) {}

PsMuxer::~PsMuxer() {}

bool PsMuxer::Initialize(StreamType videoType, StreamType audioType) {
    return impl_->Initialize(videoType, audioType);
}

bool PsMuxer::WriteH264Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts) {
    return impl_->WriteH264Nalu(data, len, pts, dts);
}

bool PsMuxer::WriteH265Nalu(const uint8_t* data, size_t len, uint64_t pts, uint64_t dts) {
    return impl_->WriteH265Nalu(data, len, pts, dts);
}

bool PsMuxer::WriteAudioData(const uint8_t* data, size_t len, uint64_t pts) {
    return impl_->WriteAudioData(data, len, pts);
}

std::vector<uint8_t> PsMuxer::GetPsData() {
    return impl_->GetPsData();
}

void PsMuxer::Clear() {
    impl_->Clear();
}

} // namespace gb28181
