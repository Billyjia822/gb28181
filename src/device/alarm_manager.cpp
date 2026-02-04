#include "device/alarm_manager.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <mutex>
#include <iostream>

namespace gb28181 {

AlarmManager::AlarmManager()
    : initialized_(false)
    , reportingEnabled_(false)
    , reportingInterval_(60)
    , running_(false)
    , alarmCounter_(0) {
}

AlarmManager::~AlarmManager() {
    StopAlarmReporting();
}

bool AlarmManager::Initialize() {
    initialized_ = true;
    std::cout << "[AlarmManager] Initialized" << std::endl;
    return true;
}

std::string AlarmManager::TriggerAlarm(const AlarmInfo& alarm) {
    if (!initialized_) {
        std::cerr << "[AlarmManager] Not initialized" << std::endl;
        return "";
    }

    // 生成告警ID
    std::string alarmId = GenerateAlarmId();

    // 创建告警副本
    AlarmInfo newAlarm = alarm;
    newAlarm.alarmId = alarmId;
    newAlarm.isActive = true;

    // 设置开始时间
    if (newAlarm.startTime.empty()) {
        newAlarm.startTime = GetCurrentTime();
    }

    // 添加到活跃告警列表
    alarms_[alarmId] = newAlarm;
    alarmHistory_.push_back(newAlarm);

    // 限制历史记录数量
    const int MAX_HISTORY = 1000;
    if (alarmHistory_.size() > MAX_HISTORY) {
        alarmHistory_.erase(alarmHistory_.begin());
    }

    std::cout << "[AlarmManager] Triggered alarm: " << alarmId
              << " type=" << GetAlarmTypeName(newAlarm.type)
              << " level=" << GetAlarmLevelName(newAlarm.level)
              << " channel=" << newAlarm.channelId << std::endl;

    // 调用回调函数
    if (alarmCallback_) {
        alarmCallback_(newAlarm);
    }

    return alarmId;
}

bool AlarmManager::ClearAlarm(const std::string& alarmId) {
    auto it = alarms_.find(alarmId);
    if (it == alarms_.end()) {
        std::cerr << "[AlarmManager] Alarm not found: " << alarmId << std::endl;
        return false;
    }

    // 更新告警状态
    it->second.isActive = false;
    it->second.endTime = GetCurrentTime();

    std::cout << "[AlarmManager] Cleared alarm: " << alarmId << std::endl;

    // 从活跃列表中移除（保留在历史记录中）
    alarms_.erase(it);

    return true;
}

std::vector<AlarmInfo> AlarmManager::GetActiveAlarms() {
    std::vector<AlarmInfo> activeAlarms;

    for (const auto& pair : alarms_) {
        if (pair.second.isActive) {
            activeAlarms.push_back(pair.second);
        }
    }

    return activeAlarms;
}

std::vector<AlarmInfo> AlarmManager::GetAlarmHistory(const std::string& channelId, int limit) {
    std::vector<AlarmInfo> history;

    for (const auto& alarm : alarmHistory_) {
        if (channelId.empty() || alarm.channelId == channelId) {
            history.push_back(alarm);
        }
    }

    // 按时间倒序排列
    std::sort(history.begin(), history.end(),
        [](const AlarmInfo& a, const AlarmInfo& b) {
            return a.startTime > b.startTime;
        });

    // 限制数量
    if (limit > 0 && history.size() > limit) {
        history.resize(limit);
    }

    return history;
}

std::string AlarmManager::GenerateAlarmNotify(const AlarmInfo& alarm) {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Notify>\r\n";
    ss << "<CmdType>Alarm</CmdType>\r\n";
    ss << "<SN>" << (++alarmCounter_) << "</SN>\r\n";
    ss << "<DeviceID>" << alarm.deviceId << "</DeviceID>\r\n";
    ss << "<AlarmPriority>" << alarm.priority << "</AlarmPriority>\r\n";
    ss << "<AlarmTime>" << alarm.startTime << "</AlarmTime>\r\n";

    // 告警方法
    ss << "<AlarmMethod>" << alarm.method << "</AlarmMethod>\r\n";

    // 告警类型
    ss << "<AlarmType>" << GetAlarmTypeName(alarm.type) << "</AlarmType>\r\n";

    // 告警级别
    ss << "<AlarmLevel>" << GetAlarmLevelName(alarm.level) << "</AlarmLevel>\r\n";

    // 通道ID
    if (!alarm.channelId.empty()) {
        ss << "<DeviceID>" << alarm.channelId << "</DeviceID>\r\n";
    }

    // 描述
    if (!alarm.description.empty()) {
        ss << "<Description>" << alarm.description << "</Description>\r\n";
    }

    // 经纬度
    if (alarm.latitude != 0 || alarm.longitude != 0) {
        ss << "<Longitude>" << alarm.longitude << "</Longitude>\r\n";
        ss << "<Latitude>" << alarm.latitude << "</Latitude>\r\n";
    }

    // 附件
    if (!alarm.attachment.empty()) {
        ss << "<Attachment>" << alarm.attachment << "</Attachment>\r\n";
    }

    ss << "</Notify>\r\n";

    return ss.str();
}

void AlarmManager::SetAlarmCallback(std::function<void(const AlarmInfo&)> callback) {
    alarmCallback_ = callback;
}

void AlarmManager::StartAlarmReporting(int interval) {
    if (reportingEnabled_) {
        std::cout << "[AlarmManager] Alarm reporting already started" << std::endl;
        return;
    }

    reportingInterval_ = interval;
    reportingEnabled_ = true;
    running_ = true;

    reportingThread_ = std::thread(&AlarmManager::AlarmReportingThread, this);

    std::cout << "[AlarmManager] Started alarm reporting with interval: " << interval << "s" << std::endl;
}

void AlarmManager::StopAlarmReporting() {
    if (!reportingEnabled_) {
        return;
    }

    running_ = false;
    reportingEnabled_ = false;

    if (reportingThread_.joinable()) {
        reportingThread_.join();
    }

    std::cout << "[AlarmManager] Stopped alarm reporting" << std::endl;
}

void AlarmManager::AlarmReportingThread() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(reportingInterval_));

        if (!running_) {
            break;
        }

        // 获取活跃告警
        auto activeAlarms = GetActiveAlarms();

        // 上报所有活跃告警
        for (const auto& alarm : activeAlarms) {
            if (alarmCallback_) {
                alarmCallback_(alarm);
            }
        }
    }
}

std::string AlarmManager::GetAlarmTypeName(AlarmType type) {
    switch (type) {
        case AlarmType::VIDEO_LOSS:      return "1";
        case AlarmType::MOTION_DETECT:   return "2";
        case AlarmType::IO_ALARM:        return "3";
        case AlarmType::STORAGE_FAILURE: return "4";
        case AlarmType::NETWORK_FAILURE: return "5";
        case AlarmType::ILLEGAL_ACCESS:  return "6";
        case AlarmType::VIDEO_BLIND:     return "7";
        default:                         return "8";
    }
}

std::string AlarmManager::GetAlarmLevelName(AlarmLevel level) {
    switch (level) {
        case AlarmLevel::INFO:      return "1";
        case AlarmLevel::WARNING:   return "2";
        case AlarmLevel::CRITICAL:  return "3";
        case AlarmLevel::EMERGENCY: return "4";
        default:                    return "1";
    }
}

std::string AlarmManager::GenerateAlarmId() {
    std::stringstream ss;
    ss << "alarm_" << std::chrono::system_clock::now().time_since_epoch().count()
       << "_" << (++alarmCounter_);
    return ss.str();
}

std::string AlarmManager::GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);

    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (tm.tm_year + 1900)
       << std::setw(2) << (tm.tm_mon + 1)
       << std::setw(2) << tm.tm_mday << "T"
       << std::setw(2) << tm.tm_hour
       << std::setw(2) << tm.tm_min
       << std::setw(2) << tm.tm_sec;

    return ss.str();
}

} // namespace gb28181
