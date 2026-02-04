#ifndef GB28181_CONFIG_MANAGER_H
#define GB28181_CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace gb28181 {

/**
 * @brief 配置类型
 */
enum class ConfigType {
    BASIC,          // 基本配置
    VIDEO,          // 视频配置
    AUDIO,          // 音频配置
    PTZ,            // 云台配置
    STORAGE,        // 存储配置
    NETWORK,        // 网络配置
    ALARM,          // 告警配置
    OSD,            // OSD配置
    PRIVACY,        // 隐私遮挡配置
    ALL             // 所有配置
};

/**
 * @brief 配置项
 */
struct ConfigItem {
    std::string key;        // 配置键
    std::string value;      // 配置值
    std::string type;       // 类型 (string, int, bool)
    std::string desc;       // 描述
};

/**
 * @brief 视频配置
 */
struct VideoConfig {
    std::string codec;          // 编码格式 (H264, H265)
    int resolution;             // 分辨率 (0=QCIF, 1=CIF, 2=D1, 3=720P, 4=1080P)
    int frameRate;              // 帧率
    int bitRate;                // 码率 (Kbps)
    int gop;                    // GOP长度
    int profileLevelId;         // H.264/H.265 Profile/Level
    bool vbr;                   // 是否可变码率
    int quality;                // 图像质量 (1-10)
};

/**
 * @brief 音频配置
 */
struct AudioConfig {
    std::string codec;          // 编码格式 (G711A, G711U, AAC)
    int sampleRate;             // 采样率 (8000, 16000, 44100)
    int channels;               // 声道数 (1=单声道, 2=立体声)
    int bitRate;                // 码率
    int volume;                 // 音量 (0-100)
};

/**
 * @brief 云台配置
 */
struct PTZConfig {
    bool enabled;               // 是否启用
    int presetCount;            // 预置位数量
    int cruiseSpeed;            // 巡航速度
    bool autoFlip;              // 自动翻转
};

/**
 * @brief 存储配置
 */
struct StorageConfig {
    std::string path;           // 存储路径
    uint64_t totalSpace;        // 总空间 (MB)
    uint64_t usedSpace;         // 已用空间 (MB)
    int recordDays;             // 录像保留天数
    bool autoDelete;            // 自动删除旧录像
};

/**
 * @brief 网络配置
 */
struct NetworkConfig {
    std::string ipAddress;      // IP地址
    std::string netmask;        // 子网掩码
    std::string gateway;        // 网关
    std::string dns;            // DNS服务器
    int mtu;                    // MTU
    bool dhcp;                  // 是否启用DHCP
};

/**
 * @brief 告警配置
 */
struct AlarmConfig {
    bool motionDetect;          // 移动侦测
    int motionSensitivity;      // 移动侦测灵敏度 (1-10)
    bool videoLoss;             // 视频丢失检测
    bool storageAlarm;          // 存储告警
    bool ioAlarm;               // IO告警
};

/**
 * @brief OSD配置
 */
struct OSDConfig {
    bool enabled;               // 是否启用
    std::string text;           // 显示文本
    int positionX;              // X坐标
    int positionY;              // Y坐标
    int fontSize;               // 字体大小
    std::string color;          // 颜色
};

/**
 * @brief 隐私遮挡配置
 */
struct PrivacyConfig {
    bool enabled;               // 是否启用
    int regionCount;            // 遮挡区域数量
};

/**
 * @brief 设备配置管理器
 * 用于GB28181设备配置功能
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    /**
     * @brief 初始化配置管理器
     * @param configPath 配置文件路径
     * @return 是否成功
     */
    bool Initialize(const std::string& configPath);

    /**
     * @brief 加载配置
     * @param type 配置类型
     * @return 是否成功
     */
    bool LoadConfig(ConfigType type = ConfigType::ALL);

    /**
     * @brief 保存配置
     * @param type 配置类型
     * @return 是否成功
     */
    bool SaveConfig(ConfigType type = ConfigType::ALL);

    /**
     * @brief 获取配置值
     * @param key 配置键
     * @return 配置值
     */
    std::string GetValue(const std::string& key);

    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     * @return 是否成功
     */
    bool SetValue(const std::string& key, const std::string& value);

    /**
     * @brief 获取视频配置
     */
    VideoConfig GetVideoConfig();

    /**
     * @brief 设置视频配置
     */
    bool SetVideoConfig(const VideoConfig& config);

    /**
     * @brief 获取音频配置
     */
    AudioConfig GetAudioConfig();

    /**
     * @brief 设置音频配置
     */
    bool SetAudioConfig(const AudioConfig& config);

    /**
     * @brief 获取云台配置
     */
    PTZConfig GetPTZConfig();

    /**
     * @brief 设置云台配置
     */
    bool SetPTZConfig(const PTZConfig& config);

    /**
     * @brief 获取存储配置
     */
    StorageConfig GetStorageConfig();

    /**
     * @brief 设置存储配置
     */
    bool SetStorageConfig(const StorageConfig& config);

    /**
     * @brief 获取网络配置
     */
    NetworkConfig GetNetworkConfig();

    /**
     * @brief 设置网络配置
     */
    bool SetNetworkConfig(const NetworkConfig& config);

    /**
     * @brief 获取告警配置
     */
    AlarmConfig GetAlarmConfig();

    /**
     * @brief 设置告警配置
     */
    bool SetAlarmConfig(const AlarmConfig& config);

    /**
     * @brief 获取OSD配置
     */
    OSDConfig GetOSDConfig();

    /**
     * @brief 设置OSD配置
     */
    bool SetOSDConfig(const OSDConfig& config);

    /**
     * @brief 获取隐私遮挡配置
     */
    PrivacyConfig GetPrivacyConfig();

    /**
     * @brief 设置隐私遮挡配置
     */
    bool SetPrivacyConfig(const PrivacyConfig& config);

    /**
     * @brief 生成设备配置响应
     * @param deviceId 设备ID
     * @param sn 序列号
     * @param configType 配置类型
     * @return MANSCDP XML响应字符串
     */
    std::string GenerateConfigResponse(const std::string& deviceId,
                                      const std::string& sn,
                                      ConfigType configType);

    /**
     * @brief 解析配置请求
     * @param xmlStr XML字符串
     * @return 配置类型
     */
    ConfigType ParseConfigRequest(const std::string& xmlStr);

    /**
     * @brief 设置配置变更回调
     * @param callback 回调函数
     */
    void SetConfigChangeCallback(std::function<void(const std::string&, const std::string&)> callback);

private:
    /**
     * @brief 从文件加载配置
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief 保存配置到文件
     */
    bool SaveToFile(const std::string& filePath);

    /**
     * @brief 解析配置类型名称
     */
    std::string GetConfigTypeName(ConfigType type);

private:
    std::string configPath_;
    std::map<std::string, std::string> configs_;
    VideoConfig videoConfig_;
    AudioConfig audioConfig_;
    PTZConfig ptzConfig_;
    StorageConfig storageConfig_;
    NetworkConfig networkConfig_;
    AlarmConfig alarmConfig_;
    OSDConfig osdConfig_;
    PrivacyConfig privacyConfig_;
    std::function<void(const std::string&, const std::string&)> configChangeCallback_;
    bool initialized_;
};

} // namespace gb28181

#endif // GB28181_CONFIG_MANAGER_H
