#ifndef GB28181_DEVICE_MANAGER_H
#define GB28181_DEVICE_MANAGER_H

#include <string>
#include <memory>
#include <functional>

namespace gb28181 {

enum class DeviceStatus {
    ONLINE,
    OFFLINE,
    REGISTERING
};

struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string manufacturer;
    std::string model;
    std::string firmwareVersion;
    std::string ipAddress;
    int port;
    DeviceStatus status;
};

struct ChannelInfo {
    std::string channelId;
    std::string channelName;
    int channelType;  // 0:主码流 1:子码流
    std::string status;
};

using DeviceEventCallback = std::function<void(const std::string& event, const std::string& data)>;

class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // 初始化设备管理器
    bool Initialize(const std::string& configPath);

    // 设置设备信息
    void SetDeviceInfo(const DeviceInfo& info);

    // 获取设备信息
    DeviceInfo GetDeviceInfo() const;

    // 添加通道
    void AddChannel(const ChannelInfo& channel);

    // 获取所有通道
    std::vector<ChannelInfo> GetAllChannels() const;

    // 设置设备状态
    void SetDeviceStatus(DeviceStatus status);

    // 获取设备状态
    DeviceStatus GetDeviceStatus() const;

    // 设置事件回调
    void SetEventCallback(DeviceEventCallback callback);

    // 生成设备目录查询响应
    std::string GenerateCatalogResponse() const;

    // 生成设备信息查询响应
    std::string GenerateDeviceInfoResponse() const;

    // 生成设备状态查询响应
    std::string GenerateDeviceStatusResponse() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gb28181

#endif // GB28181_DEVICE_MANAGER_H
