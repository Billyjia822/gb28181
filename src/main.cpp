#include "sip/sip_manager.h"
#include "device/device_manager.h"
#include "rtp/rtp_manager.h"
#include "ps/ps_muxer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

using namespace gb28181;

// 全局变量
std::unique_ptr<SipManager> g_sipManager;
std::unique_ptr<DeviceManager> g_deviceManager;
std::unique_ptr<RtpManager> g_rtpManager;
std::unique_ptr<PsMuxer> g_psMuxer;
bool g_running = true;

// 信号处理函数
void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// SIP事件回调
void OnSipEvent(const std::string& event, const std::string& data) {
    std::cout << "[SIP Event] " << event << ": " << data << std::endl;

    if (event == "REGISTER_SUCCESS") {
        std::cout << "Device registered to SIP server successfully!" << std::endl;
        g_deviceManager->SetDeviceStatus(DeviceStatus::ONLINE);
    } else if (event == "REGISTER_FAILURE") {
        std::cout << "Failed to register to SIP server!" << std::endl;
        g_deviceManager->SetDeviceStatus(DeviceStatus::OFFLINE);
    } else if (event == "INVITE_RECEIVED") {
        std::cout << "Video streaming request received!" << std::endl;
        // TODO: 启动视频流发送
    }
}

// 设备事件回调
void OnDeviceEvent(const std::string& event, const std::string& data) {
    std::cout << "[Device Event] " << event << ": " << data << std::endl;
}

// RTP接收回调
void OnRtpReceive(const RtpPacket& packet, const std::string& fromIp, int fromPort) {
    std::cout << "[RTP] Received packet from " << fromIp << ":" << fromPort
              << ", seq=" << packet.sequenceNumber
              << ", size=" << packet.payload.size() << std::endl;
}

// 发送心跳线程
void HeartbeatThread() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        if (g_sipManager) {
            g_sipManager->SendHeartbeat();
        }
    }
}

// SIP消息处理线程
void SipProcessThread() {
    while (g_running) {
        g_sipManager->ProcessMessage();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// RTP处理线程
void RtpProcessThread() {
    while (g_running) {
        g_rtpManager->Process();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "    GB28181 Device Application" << std::endl;
    std::cout << "========================================" << std::endl;

    // 设置信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // 配置参数
    std::string localIp = "auto";  // 自动检测本机IP
    int sipPort = 5060;
    std::string deviceId = "34020000001320000001";
    std::string realm = "3402000000";
    std::string serverIp = "192.168.1.1";
    int serverPort = 5060;
    std::string username = "34020000001320000001";
    std::string password = "12345678";

    // 解析命令行参数
    // 用法: ./gb28181_device [local_ip] [server_ip]
    // 示例: ./gb28181_device 192.168.1.100 192.168.1.1
    if (argc > 1) {
        localIp = argv[1];
    }
    if (argc > 2) {
        serverIp = argv[2];
    }

    std::cout << "Local IP: " << (localIp == "auto" ? "auto-detect" : localIp) << std::endl;
    std::cout << "SIP Server: " << serverIp << ":" << serverPort << std::endl;
    std::cout << "Device ID: " << deviceId << std::endl;

    // 初始化SIP管理器
    std::cout << "\nInitializing SIP Manager..." << std::endl;
    g_sipManager = std::make_unique<SipManager>();
    if (!g_sipManager->Initialize(localIp, sipPort, deviceId, realm)) {
        std::cerr << "Failed to initialize SIP Manager!" << std::endl;
        return -1;
    }
    g_sipManager->SetEventCallback(OnSipEvent);

    // 初始化设备管理器
    std::cout << "Initializing Device Manager..." << std::endl;
    g_deviceManager = std::make_unique<DeviceManager>();

    DeviceInfo deviceInfo;
    deviceInfo.deviceId = deviceId;
    deviceInfo.deviceName = "GB28181 Camera";
    deviceInfo.manufacturer = "GB28181 Inc.";
    deviceInfo.model = "IPC-1000";
    deviceInfo.firmwareVersion = "1.0.0";
    deviceInfo.ipAddress = localIp;
    deviceInfo.port = sipPort;
    deviceInfo.status = DeviceStatus::OFFLINE;
    g_deviceManager->SetDeviceInfo(deviceInfo);

    // 添加通道
    ChannelInfo channel;
    channel.channelId = deviceId;
    channel.channelName = "Camera 1";
    channel.channelType = 0;  // 主码流
    channel.status = "ON";
    g_deviceManager->AddChannel(channel);

    g_deviceManager->SetEventCallback(OnDeviceEvent);

    // 初始化RTP管理器
    std::cout << "Initializing RTP Manager..." << std::endl;
    g_rtpManager = std::make_unique<RtpManager>();
    if (!g_rtpManager->Initialize(localIp, 50000)) {
        std::cerr << "Failed to initialize RTP Manager!" << std::endl;
        return -1;
    }
    g_rtpManager->SetReceiveCallback(OnRtpReceive);

    // 初始化PS封装器
    std::cout << "Initializing PS Muxer..." << std::endl;
    g_psMuxer = std::make_unique<PsMuxer>();
    if (!g_psMuxer->Initialize(StreamType::H264, StreamType::AAC)) {
        std::cerr << "Failed to initialize PS Muxer!" << std::endl;
        return -1;
    }

    // 注册到SIP服务器
    std::cout << "\nRegistering to SIP Server..." << std::endl;
    if (!g_sipManager->RegisterToServer(serverIp, serverPort, username, password)) {
        std::cerr << "Failed to register to SIP Server!" << std::endl;
        return -1;
    }

    // 启动工作线程
    std::cout << "\nStarting worker threads..." << std::endl;
    std::thread heartbeatThread(HeartbeatThread);
    std::thread sipProcessThread(SipProcessThread);
    std::thread rtpProcessThread(RtpProcessThread);

    std::cout << "\nGB28181 Device is running. Press Ctrl+C to stop." << std::endl;

    // 主循环
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 清理资源
    std::cout << "\nShutting down..." << std::endl;

    g_sipManager->Unregister();

    g_running = false;

    if (heartbeatThread.joinable()) heartbeatThread.join();
    if (sipProcessThread.joinable()) sipProcessThread.join();
    if (rtpProcessThread.joinable()) rtpProcessThread.join();

    g_psMuxer.reset();
    g_rtpManager.reset();
    g_deviceManager.reset();
    g_sipManager.reset();

    std::cout << "Shutdown complete." << std::endl;

    return 0;
}
