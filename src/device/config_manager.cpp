#include "device/config_manager.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
namespace gb28181 {

ConfigManager::ConfigManager() : initialized_(false) {
    // 初始化默认配置
    videoConfig_ = {"H264", 4, 25, 4096, 25, 0x42E01F, false, 7};
    audioConfig_ = {"G711A", 8000, 1, 64, 80};
    ptzConfig_ = {true, 255, 5, false};
    storageConfig_ = {"/storage", 1024000, 512000, 30, true};
    networkConfig_ = {"192.168.1.100", "255.255.255.0", "192.168.1.1", "8.8.8.8", 1500, true};
    alarmConfig_ = {true, 5, true, true, false};
    osdConfig_ = {true, "Camera 1", 10, 10, 24, "white"};
    privacyConfig_ = {false, 4};
}

ConfigManager::~ConfigManager() {
    if (initialized_) {
        SaveConfig(ConfigType::ALL);
    }
}

bool ConfigManager::Initialize(const std::string& configPath) {
    configPath_ = configPath;

    // 加载配置
    if (!LoadConfig(ConfigType::ALL)) {
        std::cerr << "[ConfigManager] Failed to load config from: " << configPath_ << std::endl;
        // 不返回false，使用默认配置
    }

    initialized_ = true;
    std::cout << "[ConfigManager] Initialized with config: " << configPath_ << std::endl;

    return true;
}

bool ConfigManager::LoadConfig(ConfigType type) {
    if (!initialized_) {
        return false;
    }

    std::string filePath = configPath_;
    if (type == ConfigType::ALL) {
        filePath = configPath_;
    } else {
        filePath = configPath_ + "_" + GetConfigTypeName(type);
    }

    return LoadFromFile(filePath);
}

bool ConfigManager::SaveConfig(ConfigType type) {
    if (!initialized_) {
        return false;
    }

    std::string filePath = configPath_;
    if (type == ConfigType::ALL) {
        filePath = configPath_;
    } else {
        filePath = configPath_ + "_" + GetConfigTypeName(type);
    }

    return SaveToFile(filePath);
}

std::string ConfigManager::GetValue(const std::string& key) {
    auto it = configs_.find(key);
    return (it != configs_.end()) ? it->second : "";
}

bool ConfigManager::SetValue(const std::string& key, const std::string& value) {
    configs_[key] = value;

    // 调用回调
    if (configChangeCallback_) {
        configChangeCallback_(key, value);
    }

    std::cout << "[ConfigManager] Set config: " << key << " = " << value << std::endl;

    return true;
}

VideoConfig ConfigManager::GetVideoConfig() {
    return videoConfig_;
}

bool ConfigManager::SetVideoConfig(const VideoConfig& config) {
    videoConfig_ = config;

    // 保存到配置映射
    SetValue("video.codec", config.codec);
    SetValue("video.resolution", std::to_string(config.resolution));
    SetValue("video.frameRate", std::to_string(config.frameRate));
    SetValue("video.bitRate", std::to_string(config.bitRate));
    SetValue("video.gop", std::to_string(config.gop));
    SetValue("video.profileLevelId", std::to_string(config.profileLevelId));
    SetValue("video.vbr", config.vbr ? "true" : "false");
    SetValue("video.quality", std::to_string(config.quality));

    return SaveConfig(ConfigType::VIDEO);
}

AudioConfig ConfigManager::GetAudioConfig() {
    return audioConfig_;
}

bool ConfigManager::SetAudioConfig(const AudioConfig& config) {
    audioConfig_ = config;

    SetValue("audio.codec", config.codec);
    SetValue("audio.sampleRate", std::to_string(config.sampleRate));
    SetValue("audio.channels", std::to_string(config.channels));
    SetValue("audio.bitRate", std::to_string(config.bitRate));
    SetValue("audio.volume", std::to_string(config.volume));

    return SaveConfig(ConfigType::AUDIO);
}

PTZConfig ConfigManager::GetPTZConfig() {
    return ptzConfig_;
}

bool ConfigManager::SetPTZConfig(const PTZConfig& config) {
    ptzConfig_ = config;

    SetValue("ptz.enabled", config.enabled ? "true" : "false");
    SetValue("ptz.presetCount", std::to_string(config.presetCount));
    SetValue("ptz.cruiseSpeed", std::to_string(config.cruiseSpeed));
    SetValue("ptz.autoFlip", config.autoFlip ? "true" : "false");

    return SaveConfig(ConfigType::PTZ);
}

StorageConfig ConfigManager::GetStorageConfig() {
    return storageConfig_;
}

bool ConfigManager::SetStorageConfig(const StorageConfig& config) {
    storageConfig_ = config;

    SetValue("storage.path", config.path);
    SetValue("storage.totalSpace", std::to_string(config.totalSpace));
    SetValue("storage.usedSpace", std::to_string(config.usedSpace));
    SetValue("storage.recordDays", std::to_string(config.recordDays));
    SetValue("storage.autoDelete", config.autoDelete ? "true" : "false");

    return SaveConfig(ConfigType::STORAGE);
}

NetworkConfig ConfigManager::GetNetworkConfig() {
    return networkConfig_;
}

bool ConfigManager::SetNetworkConfig(const NetworkConfig& config) {
    networkConfig_ = config;

    SetValue("network.ipAddress", config.ipAddress);
    SetValue("network.netmask", config.netmask);
    SetValue("network.gateway", config.gateway);
    SetValue("network.dns", config.dns);
    SetValue("network.mtu", std::to_string(config.mtu));
    SetValue("network.dhcp", config.dhcp ? "true" : "false");

    return SaveConfig(ConfigType::NETWORK);
}

AlarmConfig ConfigManager::GetAlarmConfig() {
    return alarmConfig_;
}

bool ConfigManager::SetAlarmConfig(const AlarmConfig& config) {
    alarmConfig_ = config;

    SetValue("alarm.motionDetect", config.motionDetect ? "true" : "false");
    SetValue("alarm.motionSensitivity", std::to_string(config.motionSensitivity));
    SetValue("alarm.videoLoss", config.videoLoss ? "true" : "false");
    SetValue("alarm.storageAlarm", config.storageAlarm ? "true" : "false");
    SetValue("alarm.ioAlarm", config.ioAlarm ? "true" : "false");

    return SaveConfig(ConfigType::ALARM);
}

OSDConfig ConfigManager::GetOSDConfig() {
    return osdConfig_;
}

bool ConfigManager::SetOSDConfig(const OSDConfig& config) {
    osdConfig_ = config;

    SetValue("osd.enabled", config.enabled ? "true" : "false");
    SetValue("osd.text", config.text);
    SetValue("osd.positionX", std::to_string(config.positionX));
    SetValue("osd.positionY", std::to_string(config.positionY));
    SetValue("osd.fontSize", std::to_string(config.fontSize));
    SetValue("osd.color", config.color);

    return SaveConfig(ConfigType::OSD);
}

PrivacyConfig ConfigManager::GetPrivacyConfig() {
    return privacyConfig_;
}

bool ConfigManager::SetPrivacyConfig(const PrivacyConfig& config) {
    privacyConfig_ = config;

    SetValue("privacy.enabled", config.enabled ? "true" : "false");
    SetValue("privacy.regionCount", std::to_string(config.regionCount));

    return SaveConfig(ConfigType::PRIVACY);
}

std::string ConfigManager::GenerateConfigResponse(const std::string& deviceId,
                                                 const std::string& sn,
                                                 ConfigType configType) {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>DeviceConfig</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << deviceId << "</DeviceID>\r\n";

    switch (configType) {
        case ConfigType::VIDEO:
        {
            ss << "<ConfigType>Video</ConfigType>\r\n";
            ss << "<VideoConfig>\r\n";
            ss << "<Codec>" << videoConfig_.codec << "</Codec>\r\n";
            ss << "<Resolution>" << videoConfig_.resolution << "</Resolution>\r\n";
            ss << "<FrameRate>" << videoConfig_.frameRate << "</FrameRate>\r\n";
            ss << "<BitRate>" << videoConfig_.bitRate << "</BitRate>\r\n";
            ss << "<GOP>" << videoConfig_.gop << "</GOP>\r\n";
            ss << "<ProfileLevelId>" << videoConfig_.profileLevelId << "</ProfileLevelId>\r\n";
            ss << "<VBR>" << (videoConfig_.vbr ? "true" : "false") << "</VBR>\r\n";
            ss << "<Quality>" << videoConfig_.quality << "</Quality>\r\n";
            ss << "</VideoConfig>\r\n";
            break;
        }

        case ConfigType::AUDIO:
        {
            ss << "<ConfigType>Audio</ConfigType>\r\n";
            ss << "<AudioConfig>\r\n";
            ss << "<Codec>" << audioConfig_.codec << "</Codec>\r\n";
            ss << "<SampleRate>" << audioConfig_.sampleRate << "</SampleRate>\r\n";
            ss << "<Channels>" << audioConfig_.channels << "</Channels>\r\n";
            ss << "<BitRate>" << audioConfig_.bitRate << "</BitRate>\r\n";
            ss << "<Volume>" << audioConfig_.volume << "</Volume>\r\n";
            ss << "</AudioConfig>\r\n";
            break;
        }

        case ConfigType::PTZ:
        {
            ss << "<ConfigType>PTZ</ConfigType>\r\n";
            ss << "<PTZConfig>\r\n";
            ss << "<Enabled>" << (ptzConfig_.enabled ? "true" : "false") << "</Enabled>\r\n";
            ss << "<PresetCount>" << ptzConfig_.presetCount << "</PresetCount>\r\n";
            ss << "<CruiseSpeed>" << ptzConfig_.cruiseSpeed << "</CruiseSpeed>\r\n";
            ss << "<AutoFlip>" << (ptzConfig_.autoFlip ? "true" : "false") << "</AutoFlip>\r\n";
            ss << "</PTZConfig>\r\n";
            break;
        }

        case ConfigType::STORAGE:
        {
            ss << "<ConfigType>Storage</ConfigType>\r\n";
            ss << "<StorageConfig>\r\n";
            ss << "<Path>" << storageConfig_.path << "</Path>\r\n";
            ss << "<TotalSpace>" << storageConfig_.totalSpace << "</TotalSpace>\r\n";
            ss << "<UsedSpace>" << storageConfig_.usedSpace << "</UsedSpace>\r\n";
            ss << "<RecordDays>" << storageConfig_.recordDays << "</RecordDays>\r\n";
            ss << "<AutoDelete>" << (storageConfig_.autoDelete ? "true" : "false") << "</AutoDelete>\r\n";
            ss << "</StorageConfig>\r\n";
            break;
        }

        case ConfigType::NETWORK:
        {
            ss << "<ConfigType>Network</ConfigType>\r\n";
            ss << "<NetworkConfig>\r\n";
            ss << "<IPAddress>" << networkConfig_.ipAddress << "</IPAddress>\r\n";
            ss << "<Netmask>" << networkConfig_.netmask << "</Netmask>\r\n";
            ss << "<Gateway>" << networkConfig_.gateway << "</Gateway>\r\n";
            ss << "<DNS>" << networkConfig_.dns << "</DNS>\r\n";
            ss << "<MTU>" << networkConfig_.mtu << "</MTU>\r\n";
            ss << "<DHCP>" << (networkConfig_.dhcp ? "true" : "false") << "</DHCP>\r\n";
            ss << "</NetworkConfig>\r\n";
            break;
        }

        case ConfigType::ALARM:
        {
            ss << "<ConfigType>Alarm</ConfigType>\r\n";
            ss << "<AlarmConfig>\r\n";
            ss << "<MotionDetect>" << (alarmConfig_.motionDetect ? "true" : "false") << "</MotionDetect>\r\n";
            ss << "<MotionSensitivity>" << alarmConfig_.motionSensitivity << "</MotionSensitivity>\r\n";
            ss << "<VideoLoss>" << (alarmConfig_.videoLoss ? "true" : "false") << "</VideoLoss>\r\n";
            ss << "<StorageAlarm>" << (alarmConfig_.storageAlarm ? "true" : "false") << "</StorageAlarm>\r\n";
            ss << "<IOAlarm>" << (alarmConfig_.ioAlarm ? "true" : "false") << "</IOAlarm>\r\n";
            ss << "</AlarmConfig>\r\n";
            break;
        }

        case ConfigType::OSD:
        {
            ss << "<ConfigType>OSD</ConfigType>\r\n";
            ss << "<OSDConfig>\r\n";
            ss << "<Enabled>" << (osdConfig_.enabled ? "true" : "false") << "</Enabled>\r\n";
            ss << "<Text>" << osdConfig_.text << "</Text>\r\n";
            ss << "<PositionX>" << osdConfig_.positionX << "</PositionX>\r\n";
            ss << "<PositionY>" << osdConfig_.positionY << "</PositionY>\r\n";
            ss << "<FontSize>" << osdConfig_.fontSize << "</FontSize>\r\n";
            ss << "<Color>" << osdConfig_.color << "</Color>\r\n";
            ss << "</OSDConfig>\r\n";
            break;
        }

        default:
            ss << "<ConfigType>Basic</ConfigType>\r\n";
            break;
    }

    ss << "</Response>\r\n";

    return ss.str();
}

ConfigType ConfigManager::ParseConfigRequest(const std::string& xmlStr) {
    size_t pos = xmlStr.find("<ConfigType>");
    if (pos != std::string::npos) {
        pos += 12;
        size_t endPos = xmlStr.find("</ConfigType>", pos);
        if (endPos != std::string::npos) {
            std::string typeStr = xmlStr.substr(pos, endPos - pos);

            if (typeStr == "Video") return ConfigType::VIDEO;
            if (typeStr == "Audio") return ConfigType::AUDIO;
            if (typeStr == "PTZ") return ConfigType::PTZ;
            if (typeStr == "Storage") return ConfigType::STORAGE;
            if (typeStr == "Network") return ConfigType::NETWORK;
            if (typeStr == "Alarm") return ConfigType::ALARM;
            if (typeStr == "OSD") return ConfigType::OSD;
            if (typeStr == "Privacy") return ConfigType::PRIVACY;
        }
    }

    return ConfigType::BASIC;
}

void ConfigManager::SetConfigChangeCallback(std::function<void(const std::string&, const std::string&)> callback) {
    configChangeCallback_ = callback;
}

bool ConfigManager::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Failed to open config file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析 key=value
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            configs_[key] = value;
        }
    }

    file.close();

    // 更新结构化配置
    videoConfig_.codec = GetValue("video.codec");
    audioConfig_.codec = GetValue("audio.codec");
    // ... 其他配置更新

    std::cout << "[ConfigManager] Loaded " << configs_.size() << " config items from: " << filePath << std::endl;

    return true;
}

bool ConfigManager::SaveToFile(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Failed to open config file for writing: " << filePath << std::endl;
        return false;
    }

    file << "# GB28181 Device Configuration\n";
    file << "# Generated automatically\n\n";

    for (const auto& pair : configs_) {
        file << pair.first << "=" << pair.second << "\n";
    }

    file.close();

    std::cout << "[ConfigManager] Saved " << configs_.size() << " config items to: " << filePath << std::endl;

    return true;
}

std::string ConfigManager::GetConfigTypeName(ConfigType type) {
    switch (type) {
        case ConfigType::BASIC:    return "Basic";
        case ConfigType::VIDEO:    return "Video";
        case ConfigType::AUDIO:    return "Audio";
        case ConfigType::PTZ:      return "PTZ";
        case ConfigType::STORAGE:  return "Storage";
        case ConfigType::NETWORK:  return "Network";
        case ConfigType::ALARM:    return "Alarm";
        case ConfigType::OSD:      return "OSD";
        case ConfigType::PRIVACY:  return "Privacy";
        default:                   return "All";
    }
}

} // namespace gb28181
