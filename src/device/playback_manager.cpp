#include "device/playback_manager.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <iostream>
#include <cmath>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace gb28181 {

PlaybackManager::PlaybackManager() : initialized_(false), sessionCounter_(0) {
}

PlaybackManager::~PlaybackManager() {
    // 停止所有活跃会话
    for (auto& pair : sessions_) {
        if (pair.second && pair.second->isActive) {
            StopPlayback(pair.first);
        }
    }
}

bool PlaybackManager::Initialize() {
    initialized_ = true;
    std::cout << "[PlaybackManager] Initialized" << std::endl;
    return true;
}

std::string PlaybackManager::StartPlayback(const std::string& channelId,
                                          const std::string& startTime,
                                          const std::string& endTime,
                                          const std::string& filePath) {
    if (!initialized_) {
        std::cerr << "[PlaybackManager] Not initialized" << std::endl;
        return "";
    }

    // 检查文件是否存在
    if (!FileExists(filePath)) {
        std::cerr << "[PlaybackManager] File not found: " << filePath << std::endl;
        return "";
    }

    // 创建新会话
    std::string sessionId = GenerateSessionId();
    auto session = std::make_unique<PlaybackSession>();

    session->sessionId = sessionId;
    session->channelId = channelId;
    session->filePath = filePath;
    session->startTime = ParseTimeToMs(startTime);
    session->endTime = ParseTimeToMs(endTime);
    session->currentPosition = session->startTime;
    session->control.mode = PlaybackMode::NORMAL;
    session->control.speed = 1.0;
    session->control.position = session->startTime;
    session->control.isAudio = true;
    session->isActive = true;

    sessions_[sessionId] = std::move(session);

    std::cout << "[PlaybackManager] Started playback session: " << sessionId
              << " for channel: " << channelId
              << " from " << startTime << " to " << endTime << std::endl;

    return sessionId;
}

bool PlaybackManager::StopPlayback(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[PlaybackManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    it->second->isActive = false;

    std::cout << "[PlaybackManager] Stopped playback session: " << sessionId << std::endl;

    sessions_.erase(it);
    return true;
}

bool PlaybackManager::ControlPlayback(const std::string& sessionId, const PlaybackControl& control) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[PlaybackManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    auto& session = it->second;

    // 更新控制参数
    session->control = control;

    // 根据模式设置速度
    switch (control.mode) {
        case PlaybackMode::NORMAL:
            session->control.speed = 1.0;
            break;
        case PlaybackMode::PAUSE:
            session->control.speed = 0.0;
            break;
        case PlaybackMode::FORWARD:
            if (control.speed <= 0) {
                session->control.speed = 2.0;
            }
            break;
        case PlaybackMode::BACKWARD:
            if (control.speed >= 0) {
                session->control.speed = -2.0;
            }
            break;
        case PlaybackMode::STEP:
            session->control.speed = 0.0;
            break;
    }

    std::cout << "[PlaybackManager] Controlled playback: " << sessionId
              << " mode=" << (int)control.mode
              << " speed=" << control.speed << std::endl;

    return true;
}

bool PlaybackManager::PausePlayback(const std::string& sessionId) {
    PlaybackControl control;
    control.mode = PlaybackMode::PAUSE;
    control.speed = 0.0;

    return ControlPlayback(sessionId, control);
}

bool PlaybackManager::ResumePlayback(const std::string& sessionId) {
    PlaybackControl control;
    control.mode = PlaybackMode::NORMAL;
    control.speed = 1.0;

    return ControlPlayback(sessionId, control);
}

bool PlaybackManager::FastForward(const std::string& sessionId, double speed) {
    PlaybackControl control;
    control.mode = PlaybackMode::FORWARD;
    control.speed = speed;

    return ControlPlayback(sessionId, control);
}

bool PlaybackManager::FastBackward(const std::string& sessionId, double speed) {
    PlaybackControl control;
    control.mode = PlaybackMode::BACKWARD;
    control.speed = -speed;

    return ControlPlayback(sessionId, control);
}

bool PlaybackManager::SeekPlayback(const std::string& sessionId, uint64_t position) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[PlaybackManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    auto& session = it->second;

    // 检查位置是否在有效范围内
    if (position < session->startTime || position > session->endTime) {
        std::cerr << "[PlaybackManager] Invalid position: " << position << std::endl;
        return false;
    }

    session->currentPosition = position;
    session->control.position = position;

    std::cout << "[PlaybackManager] Seek playback: " << sessionId
              << " to position: " << position << "ms ("
              << FormatMsToTime(position) << ")" << std::endl;

    return true;
}

PlaybackSession* PlaybackManager::GetSession(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> PlaybackManager::GetActiveSessions() {
    std::vector<std::string> activeSessions;

    for (const auto& pair : sessions_) {
        if (pair.second && pair.second->isActive) {
            activeSessions.push_back(pair.first);
        }
    }

    return activeSessions;
}

void PlaybackManager::SetFrameCallback(std::function<void(const uint8_t* data, size_t len, uint64_t timestamp)> callback) {
    frameCallback_ = callback;
}

bool PlaybackManager::ReadNextFrame(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return false;
    }

    auto& session = it->second;

    if (!session->isActive) {
        return false;
    }

    // 检查是否到达结束时间
    if (session->currentPosition >= session->endTime) {
        std::cout << "[PlaybackManager] Reached end of playback: " << sessionId << std::endl;
        return false;
    }

    // 检查是否暂停
    if (session->control.mode == PlaybackMode::PAUSE) {
        return true;
    }

    // 模拟读取帧（实际应该从文件解码）
    // TODO: 实现实际的文件读取和解码
    uint8_t dummyFrame[1024] = {0};
    uint64_t timestamp = session->currentPosition;

    // 调用帧回调
    if (frameCallback_) {
        frameCallback_(dummyFrame, sizeof(dummyFrame), timestamp);
    }

    // 更新位置
    double frameTime = 40.0 / fabs(session->control.speed); // 假设25fps，每帧40ms
    if (session->control.speed < 0) {
        session->currentPosition -= (uint64_t)frameTime;
    } else {
        session->currentPosition += (uint64_t)frameTime;
    }

    return true;
}

bool PlaybackManager::FileExists(const std::string& filePath) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(filePath.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
#endif
}

std::string PlaybackManager::GenerateSessionId() {
    std::stringstream ss;
    ss << "playback_" << std::chrono::system_clock::now().time_since_epoch().count()
       << "_" << (++sessionCounter_);
    return ss.str();
}

uint64_t PlaybackManager::ParseTimeToMs(const std::string& timeStr) {
    // 时间格式: 20240101T120000 或 2024-01-01T12:00:00
    uint64_t ms = 0;

    try {
        std::tm tm = {};
        std::istringstream iss(timeStr);

        // 尝试解析格式1: 20240101T120000
        if (timeStr.find('T') != std::string::npos && timeStr.length() == 15) {
            int year, month, day, hour, minute, second;
            sscanf(timeStr.c_str(), "%04d%02d%02dT%02d%02d%02d",
                   &year, &month, &day, &hour, &minute, &second);

            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;

            auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timePoint.time_since_epoch()).count();
        }
        // 尝试解析格式2: 2024-01-01T12:00:00
        else if (timeStr.find('-') != std::string::npos) {
            int year, month, day, hour, minute, second;
            char sep1, sep2, sep3, sep4;
            iss >> year >> sep1 >> month >> sep2 >> day >> sep3
                >> hour >> sep4 >> minute >> sep4 >> second;

            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;

            auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timePoint.time_since_epoch()).count();
        }
    } catch (...) {
        std::cerr << "[PlaybackManager] Failed to parse time: " << timeStr << std::endl;
    }

    return ms;
}

std::string PlaybackManager::FormatMsToTime(uint64_t ms) {
    auto timePoint = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::milliseconds(ms)));

    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::localtime(&time);

    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (tm.tm_year + 1900) << "-"
       << std::setw(2) << (tm.tm_mon + 1) << "-"
       << std::setw(2) << tm.tm_mday << "T"
       << std::setw(2) << tm.tm_hour << ":"
       << std::setw(2) << tm.tm_min << ":"
       << std::setw(2) << tm.tm_sec;

    return ss.str();
}

} // namespace gb28181
