#include "sip/sip_transport.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace gb28181 {

class SipTransport::Impl {
public:
    Impl() : socket_(-1), running_(false) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~Impl() {
        Stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool Start(const std::string& localIp, int localPort) {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(localIp.c_str());
        addr.sin_port = htons(localPort);

        if (bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to bind socket" << std::endl;
#ifdef _WIN32
            closesocket(socket_);
#else
            close(socket_);
#endif
            return false;
        }

        running_ = true;
        localIp_ = localIp;
        localPort_ = localPort;
        return true;
    }

    void Stop() {
        if (socket_ >= 0) {
#ifdef _WIN32
            closesocket(socket_);
#else
            close(socket_);
#endif
            socket_ = -1;
        }
        running_ = false;
    }

    bool Send(const std::string& data, const std::string& destIp, int destPort) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(destIp.c_str());
        addr.sin_port = htons(destPort);

        ssize_t sent = sendto(socket_, data.c_str(), data.size(), 0,
                            (struct sockaddr*)&addr, sizeof(addr));
        return sent == data.size();
    }

    void Process() {
        if (!running_) return;

        char buffer[4096];
        struct sockaddr_in fromAddr;
        socklen_t fromLen = sizeof(fromAddr);

        ssize_t received = recvfrom(socket_, buffer, sizeof(buffer) - 1, 0,
                                   (struct sockaddr*)&fromAddr, &fromLen);

        if (received > 0 && receiveCallback_) {
            buffer[received] = '\0';
            std::string fromIp = inet_ntoa(fromAddr.sin_addr);
            int fromPort = ntohs(fromAddr.sin_port);
            receiveCallback_(std::string(buffer, received), fromIp, fromPort);
        }
    }

    void SetReceiveCallback(TransportReceiveCallback callback) {
        receiveCallback_ = callback;
    }

private:
    int socket_;
    bool running_;
    std::string localIp_;
    int localPort_;
    TransportReceiveCallback receiveCallback_;
};

SipTransport::SipTransport() : impl_(new Impl()) {}

SipTransport::~SipTransport() {
    delete impl_;
}

bool SipTransport::Start(const std::string& localIp, int localPort) {
    return impl_->Start(localIp, localPort);
}

void SipTransport::Stop() {
    impl_->Stop();
}

bool SipTransport::Send(const std::string& data, const std::string& destIp, int destPort) {
    return impl_->Send(data, destIp, destPort);
}

void SipTransport::SetReceiveCallback(TransportReceiveCallback callback) {
    impl_->SetReceiveCallback(callback);
}

void SipTransport::Process() {
    impl_->Process();
}

} // namespace gb28181
