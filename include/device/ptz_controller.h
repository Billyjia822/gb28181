#ifndef GB28181_PTZ_CONTROLLER_H
#define GB28181_PTZ_CONTROLLER_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <map>

namespace gb28181 {

/**
 * @brief PTZ命令类型
 */
enum class PTZCommand {
    STOP,           // 停止
    UP,             // 上
    DOWN,           // 下
    LEFT,           // 左
    RIGHT,          // 右
    UP_LEFT,        // 左上
    UP_RIGHT,       // 右上
    DOWN_LEFT,      // 左下
    DOWN_RIGHT,     // 右下
    ZOOM_IN,        // 放大
    ZOOM_OUT,       // 缩小
    FOCUS_NEAR,     // 焦点近
    FOCUS_FAR,      // 焦点远
    IRIS_OPEN,      // 光圈开
    IRIS_CLOSE,     // 光圈关
    PRESET_SET,     // 设置预置位
    PRESET_CALL,    // 调用预置位
    PRESET_DELETE,  // 删除预置位
    CRUISE_START,   // 开始巡航
    CRUISE_STOP,    // 停止巡航
    CRUISE_ADD,     // 添加巡航点
    SCAN_START,     // 开始扫描
    SCAN_STOP       // 停止扫描
};

/**
 * @brief PTZ控制参数
 */
struct PTZControlParams {
    PTZCommand command;        // 命令类型
    int speed;                 // 速度 (1-255)
    int presetId;              // 预置位ID (1-255)
    int cruiseId;              // 巡航ID
    int dwellTime;             // 停留时间 (秒)
    std::string channelId;     // 通道ID
};

/**
 * @brief PTZ预置位信息
 */
struct PTZPreset {
    int presetId;              // 预置位ID
    std::string name;          // 预置位名称
    double pan;                // 水平角度 (-180 ~ 180)
    double tilt;               // 垂直角度 (-90 ~ 90)
    double zoom;               // 变焦倍数 (1 ~ 32)
    double focus;              // 焦点值
    bool valid;                // 是否有效
};

/**
 * @brief PTZ巡航路径
 */
struct PTZCruisePath {
    int cruiseId;              // 巡航ID
    std::string name;          // 巡航名称
    std::vector<int> presetIds; // 预置位ID列表
    std::vector<int> speeds;   // 速度列表
    std::vector<int> dwellTimes; // 停留时间列表
    bool enabled;              // 是否启用
};

/**
 * @brief PTZ控制器
 * 用于GB28181云台控制功能
 */
class PTZController {
public:
    PTZController();
    ~PTZController();

    /**
     * @brief 初始化PTZ控制器
     * @return 是否成功
     */
    bool Initialize();

    /**
     * @brief 执行PTZ控制命令
     * @param params 控制参数
     * @return 是否成功
     */
    bool ExecuteCommand(const PTZControlParams& params);

    /**
     * @brief 停止所有PTZ动作
     * @param channelId 通道ID
     * @return 是否成功
     */
    bool StopAll(const std::string& channelId);

    /**
     * @brief 设置预置位
     * @param channelId 通道ID
     * @param presetId 预置位ID
     * @param name 预置位名称
     * @return 是否成功
     */
    bool SetPreset(const std::string& channelId, int presetId, const std::string& name = "");

    /**
     * @brief 调用预置位
     * @param channelId 通道ID
     * @param presetId 预置位ID
     * @param speed 速度
     * @return 是否成功
     */
    bool CallPreset(const std::string& channelId, int presetId, int speed = 128);

    /**
     * @brief 删除预置位
     * @param channelId 通道ID
     * @param presetId 预置位ID
     * @return 是否成功
     */
    bool DeletePreset(const std::string& channelId, int presetId);

    /**
     * @brief 获取预置位信息
     * @param channelId 通道ID
     * @param presetId 预置位ID
     * @return 预置位信息
     */
    PTZPreset GetPreset(const std::string& channelId, int presetId);

    /**
     * @brief 获取所有预置位
     * @param channelId 通道ID
     * @return 预置位列表
     */
    std::vector<PTZPreset> GetAllPresets(const std::string& channelId);

    /**
     * @brief 添加巡航点
     * @param channelId 通道ID
     * @param cruiseId 巡航ID
     * @param presetId 预置位ID
     * @param speed 速度
     * @param dwellTime 停留时间
     * @return 是否成功
     */
    bool AddCruisePoint(const std::string& channelId, int cruiseId, int presetId,
                       int speed = 128, int dwellTime = 5);

    /**
     * @brief 删除巡航点
     * @param channelId 通道ID
     * @param cruiseId 巡航ID
     * @param presetId 预置位ID
     * @return 是否成功
     */
    bool DeleteCruisePoint(const std::string& channelId, int cruiseId, int presetId);

    /**
     * @brief 开始巡航
     * @param channelId 通道ID
     * @param cruiseId 巡航ID
     * @return 是否成功
     */
    bool StartCruise(const std::string& channelId, int cruiseId);

    /**
     * @brief 停止巡航
     * @param channelId 通道ID
     * @param cruiseId 巡航ID
     * @return 是否成功
     */
    bool StopCruise(const std::string& channelId, int cruiseId);

    /**
     * @brief 开始扫描
     * @param channelId 通道ID
     * @return 是否成功
     */
    bool StartScan(const std::string& channelId);

    /**
     * @brief 停止扫描
     * @param channelId 通道ID
     * @return 是否成功
     */
    bool StopScan(const std::string& channelId);

    /**
     * @brief 解析PTZ控制命令字符串
     * @param cmdStr 命令字符串 (如 "Command=1&Speed=128")
     * @param params 输出参数
     * @return 是否成功
     */
    bool ParsePTZCommand(const std::string& cmdStr, PTZControlParams& params);

    /**
     * @brief 从XML解析PTZ命令
     * @param xmlStr XML字符串
     * @param params 输出参数
     * @return 是否成功
     */
    bool ParsePTZCommandFromXML(const std::string& xmlStr, PTZControlParams& params);

    /**
     * @brief 设置硬件控制回调
     * @param callback 回调函数
     */
    void SetHardwareCallback(std::function<bool(const PTZControlParams&)> callback);

    /**
     * @brief 获取当前PTZ状态
     * @param channelId 通道ID
     * @return 状态字符串
     */
    std::string GetPTZStatus(const std::string& channelId);

private:
    /**
     * @brief 执行硬件控制
     */
    bool ExecuteHardwareControl(const PTZControlParams& params);

    /**
     * @brief 解析命令编号
     */
    PTZCommand ParseCommandCode(int code);

    /**
     * @brief 获取命令名称
     */
    std::string GetCommandName(PTZCommand command);

private:
    std::map<std::string, std::map<int, PTZPreset>> presets_;    // 预置位: channelId -> presetId -> preset
    std::map<std::string, std::map<int, PTZCruisePath>> cruises_; // 巡航: channelId -> cruiseId -> cruise
    std::function<bool(const PTZControlParams&)> hardwareCallback_;
    bool initialized_;
    std::map<std::string, PTZControlParams> currentActions_;    // 当前动作: channelId -> params
};

} // namespace gb28181

#endif // GB28181_PTZ_CONTROLLER_H
