#include "rtp/rtp_manager.h"
#include <iostream>

namespace gb28181 {

class RtpManager::Impl {
public:
    Impl() : initialized_(false), sessionStarted_(false), ssrc_(0) {}

    bool Initialize(const std::string& localIp, int basePort) {
        localIp_ = localIp;
        basePort_ = basePort;
        initialized_ = true;
        return true;
    }

    bool StartSession(const std::string& remoteIp, int remotePort, RtpPayloadType payloadType) {
        if (!initialized_) {
            return false;
        }
        remoteIp_ = remoteIp;
        remotePort_ = remotePort;
        payloadType_ = payloadType;
        sessionStarted_ = true;
        std::cout << "[RTP] Session started to " << remoteIp << ":" << remotePort << std::endl;
        return true;
    }

    void StopSession() {
        sessionStarted_ = false;
    }

    bool SendPacket(const uint8_t* data, size_t len, bool marker) {
        if (!sessionStarted_) {
            return false;
        }
        // TODO: 实现RTP包发送
        return true;
    }

    bool SendPsData(const uint8_t* data, size_t len) {
        if (!sessionStarted_) {
            return false;
        }
        // TODO: 实现PS流数据发送
        return true;
    }

    void SetReceiveCallback(RtpReceiveCallback callback) {
        receiveCallback_ = callback;
    }

    void Process() {
        // TODO: 实现RTP接收处理
    }

    uint32_t GetSsrc() const {
        return ssrc_;
    }

    void SetSsrc(uint32_t ssrc) {
        ssrc_ = ssrc;
    }

private:
    bool initialized_;
    bool sessionStarted_;
    std::string localIp_;
    int basePort_;
    std::string remoteIp_;
    int remotePort_;
    RtpPayloadType payloadType_;
    uint32_t ssrc_;
    RtpReceiveCallback receiveCallback_;
};

RtpManager::RtpManager() : impl_(new Impl()) {}

RtpManager::~RtpManager() {
    delete impl_;
}

bool RtpManager::Initialize(const std::string& localIp, int basePort) {
    return impl_->Initialize(localIp, basePort);
}

bool RtpManager::StartSession(const std::string& remoteIp, int remotePort, RtpPayloadType payloadType) {
    return impl_->StartSession(remoteIp, remotePort, payloadType);
}

void RtpManager::StopSession() {
    impl_->StopSession();
}

bool RtpManager::SendPacket(const uint8_t* data, size_t len, bool marker) {
    return impl_->SendPacket(data, len, marker);
}

bool RtpManager::SendPsData(const uint8_t* data, size_t len) {
    return impl_->SendPsData(data, len);
}

void RtpManager::SetReceiveCallback(RtpReceiveCallback callback) {
    impl_->SetReceiveCallback(callback);
}

void RtpManager::Process() {
    impl_->Process();
}

uint32_t RtpManager::GetSsrc() const {
    return impl_->GetSsrc();
}

void RtpManager::SetSsrc(uint32_t ssrc) {
    impl_->SetSsrc(ssrc);
}

} // namespace gb28181
