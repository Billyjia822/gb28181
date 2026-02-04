#include "device/device_manager.h"
#include <sstream>
#include <map>
#include <vector>

namespace gb28181 {

class DeviceManager::Impl {
public:
    Impl() : status_(DeviceStatus::OFFLINE) {}

    bool Initialize(const std::string& configPath) {
        // TODO: 从配置文件加载设备信息
        return true;
    }

    void SetDeviceInfo(const DeviceInfo& info) {
        deviceInfo_ = info;
    }

    DeviceInfo GetDeviceInfo() const {
        return deviceInfo_;
    }

    void AddChannel(const ChannelInfo& channel) {
        channels_[channel.channelId] = channel;
    }

    std::vector<ChannelInfo> GetAllChannels() const {
        std::vector<ChannelInfo> result;
        for (const auto& pair : channels_) {
            result.push_back(pair.second);
        }
        return result;
    }

    void SetDeviceStatus(DeviceStatus status) {
        status_ = status;
        if (eventCallback_) {
            std::string statusStr = (status == DeviceStatus::ONLINE) ? "ONLINE" :
                                  (status == DeviceStatus::OFFLINE) ? "OFFLINE" : "REGISTERING";
            eventCallback_("STATUS_CHANGED", statusStr);
        }
    }

    DeviceStatus GetDeviceStatus() const {
        return status_;
    }

    void SetEventCallback(DeviceEventCallback callback) {
        eventCallback_ = callback;
    }

    std::string GenerateCatalogResponse() const {
        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>Catalog</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceInfo_.deviceId << "</DeviceID>\r\n";
        ss << "<SumNum>" << channels_.size() << "</SumNum>\r\n";
        ss << "<DeviceList Num=\"" << channels_.size() << "\">\r\n";

        int index = 1;
        for (const auto& pair : channels_) {
            const ChannelInfo& channel = pair.second;
            ss << "<Item>\r\n";
            ss << "<DeviceID>" << channel.channelId << "</DeviceID>\r\n";
            ss << "<Name>" << channel.channelName << "</Name>\r\n";
            ss << "<Manufacturer>" << deviceInfo_.manufacturer << "</Manufacturer>\r\n";
            ss << "<Model>" << deviceInfo_.model << "</Model>\r\n";
            ss << "<Status>" << channel.status << "</Status>\r\n";
            ss << "<IPAddress>" << deviceInfo_.ipAddress << "</IPAddress>\r\n";
            ss << "<Port>" << deviceInfo_.port << "</Port>\r\n";
            ss << "</Item>\r\n";
            index++;
        }

        ss << "</DeviceList>\r\n";
        ss << "</Response>\r\n";
        return ss.str();
    }

    std::string GenerateDeviceInfoResponse() const {
        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>DeviceInfo</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceInfo_.deviceId << "</DeviceID>\r\n";
        ss << "<DeviceName>" << deviceInfo_.deviceName << "</DeviceName>\r\n";
        ss << "<Manufacturer>" << deviceInfo_.manufacturer << "</Manufacturer>\r\n";
        ss << "<Model>" << deviceInfo_.model << "</Model>\r\n";
        ss << "<FirmwareVersion>" << deviceInfo_.firmwareVersion << "</FirmwareVersion>\r\n";
        ss << "</Response>\r\n";
        return ss.str();
    }

    std::string GenerateDeviceStatusResponse() const {
        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>DeviceStatus</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceInfo_.deviceId << "</DeviceID>\r\n";
        ss << "<Result>OK</Result>\r\n";
        ss << "<Online>" << ((status_ == DeviceStatus::ONLINE) ? "ONLINE" : "OFFLINE") << "</Online>\r\n";
        ss << "<Status>OK</Status>\r\n";
        ss << "</Response>\r\n";
        return ss.str();
    }

private:
    DeviceInfo deviceInfo_;
    std::map<std::string, ChannelInfo> channels_;
    DeviceStatus status_;
    DeviceEventCallback eventCallback_;
};

DeviceManager::DeviceManager() : impl_(new Impl()) {}

DeviceManager::~DeviceManager() {}

bool DeviceManager::Initialize(const std::string& configPath) {
    return impl_->Initialize(configPath);
}

void DeviceManager::SetDeviceInfo(const DeviceInfo& info) {
    impl_->SetDeviceInfo(info);
}

DeviceInfo DeviceManager::GetDeviceInfo() const {
    return impl_->GetDeviceInfo();
}

void DeviceManager::AddChannel(const ChannelInfo& channel) {
    impl_->AddChannel(channel);
}

std::vector<ChannelInfo> DeviceManager::GetAllChannels() const {
    return impl_->GetAllChannels();
}

void DeviceManager::SetDeviceStatus(DeviceStatus status) {
    impl_->SetDeviceStatus(status);
}

DeviceStatus DeviceManager::GetDeviceStatus() const {
    return impl_->GetDeviceStatus();
}

void DeviceManager::SetEventCallback(DeviceEventCallback callback) {
    impl_->SetEventCallback(callback);
}

std::string DeviceManager::GenerateCatalogResponse() const {
    return impl_->GenerateCatalogResponse();
}

std::string DeviceManager::GenerateDeviceInfoResponse() const {
    return impl_->GenerateDeviceInfoResponse();
}

std::string DeviceManager::GenerateDeviceStatusResponse() const {
    return impl_->GenerateDeviceStatusResponse();
}

} // namespace gb28181
