#ifndef GB28181_ALARM_MANAGER_H
#define GB28181_ALARM_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <map>
#include <thread>
#include <atomic>

namespace gb28181 {

/**
 * @brief 告警级别
 */
enum class AlarmLevel {
    INFO,      // 信息
    WARNING,   // 警告
    CRITICAL,  // 严重
    EMERGENCY  // 紧急
};

/**
 * @brief 告警类型
 */
enum class AlarmType {
    VIDEO_LOSS,       // 视频丢失
    MOTION_DETECT,    // 移动侦测
    IO_ALARM,         // IO告警
    STORAGE_FAILURE,  // 存储故障
    NETWORK_FAILURE,  // 网络故障
    ILLEGAL_ACCESS,   // 非法访问
    VIDEO_BLIND,      // 视频遮挡
    OTHER             // 其他
};

/**
 * @brief 告警信息
 */
struct AlarmInfo {
    std::string alarmId;        // 告警ID
    std::string deviceId;       // 设备ID
    std::string channelId;      // 通道ID
    AlarmType type;             // 告警类型
    AlarmLevel level;           // 告警级别
    std::string method;         // 告警方法 (0=即时, 1=手动, 2=防区)
    std::string startTime;      // 开始时间
    std::string endTime;        // 结束时间 (空表示持续中)
    std::string description;    // 告警描述
    double latitude;           // 纬度
    double longitude;          // 经度
    int priority;              // 优先级
    std::string attachment;     // 附件信息（如图片URL）
    bool isActive;             // 是否活跃
};

/**
 * @brief 告警管理器
 * 用于GB28181告警上报功能
 */
class AlarmManager {
public:
    AlarmManager();
    ~AlarmManager();

    /**
     * @brief 初始化告警管理器
     * @return 是否成功
     */
    bool Initialize();

    /**
     * @brief 触发告警
     * @param alarm 告警信息
     * @return 告警ID
     */
    std::string TriggerAlarm(const AlarmInfo& alarm);

    /**
     * @brief 清除告警
     * @param alarmId 告警ID
     * @return 是否成功
     */
    bool ClearAlarm(const std::string& alarmId);

    /**
     * @brief 获取活跃告警
     * @return 活跃告警列表
     */
    std::vector<AlarmInfo> GetActiveAlarms();

    /**
     * @brief 获取告警历史
     * @param channelId 通道ID (空表示所有通道)
     * @param limit 最大数量
     * @return 告警历史列表
     */
    std::vector<AlarmInfo> GetAlarmHistory(const std::string& channelId = "", int limit = 100);

    /**
     * @brief 生成告警通知XML
     * @param alarm 告警信息
     * @return MANSCDP XML字符串
     */
    std::string GenerateAlarmNotify(const AlarmInfo& alarm);

    /**
     * @brief 设置告警上报回调
     * @param callback 回调函数
     */
    void SetAlarmCallback(std::function<void(const AlarmInfo&)> callback);

    /**
     * @brief 启动告警上报
     * @param interval 上报间隔(秒)
     */
    void StartAlarmReporting(int interval = 60);

    /**
     * @brief 停止告警上报
     */
    void StopAlarmReporting();

    /**
     * @brief 获取告警类型名称
     */
    static std::string GetAlarmTypeName(AlarmType type);

    /**
     * @brief 获取告警级别名称
     */
    static std::string GetAlarmLevelName(AlarmLevel level);

private:
    /**
     * @brief 生成告警ID
     */
    std::string GenerateAlarmId();

    /**
     * @brief 获取当前时间字符串
     */
    std::string GetCurrentTime();

    /**
     * @brief 告警上报线程
     */
    void AlarmReportingThread();

private:
    std::map<std::string, AlarmInfo> alarms_;
    std::vector<AlarmInfo> alarmHistory_;
    std::function<void(const AlarmInfo&)> alarmCallback_;
    bool initialized_;
    bool reportingEnabled_;
    int reportingInterval_;
    std::thread reportingThread_;
    std::atomic<bool> running_;
    int alarmCounter_;
};

} // namespace gb28181

#endif // GB28181_ALARM_MANAGER_H
