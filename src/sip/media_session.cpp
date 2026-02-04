#include "sip/media_session.h"
#include <iostream>
#include <random>
#include <algorithm>

namespace gb28181 {

MediaSessionManager::MediaSessionManager()
    : initialized_(false)
    , ssrcCounter_(0) {
}

MediaSessionManager::~MediaSessionManager() {
    // 清理所有会话
    sessions_.clear();
}

bool MediaSessionManager::Initialize() {
    initialized_ = true;
    std::cout << "[MediaSessionManager] Initialized" << std::endl;
    return true;
}

MediaSessionInfo* MediaSessionManager::CreateSession(const std::string& sessionId,
                                                     const std::string& channelId,
                                                     const std::string& remoteIp,
                                                     const std::string& videoCodec,
                                                     const std::string& audioCodec) {
    if (!initialized_) {
        std::cerr << "[MediaSessionManager] Not initialized" << std::endl;
        return nullptr;
    }

    // 检查会话是否已存在
    if (sessions_.find(sessionId) != sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session already exists: " << sessionId << std::endl;
        return nullptr;
    }

    // 创建新会话
    auto session = std::make_unique<MediaSessionInfo>();
    session->sessionId = sessionId;
    session->channelId = channelId;
    session->remoteIp = remoteIp;
    session->remoteVideoPort = 0;
    session->remoteAudioPort = 0;
    session->localIp = "";
    session->localVideoPort = 0;
    session->localAudioPort = 0;
    session->mediaType = MediaType::VIDEO_AUDIO;
    session->state = SessionState::INVITING;
    session->videoCodec = videoCodec;
    session->audioCodec = audioCodec;
    session->videoSsrc = GenerateSsrc();
    session->audioSsrc = GenerateSsrc();
    session->createTime = std::chrono::system_clock::now();
    session->lastActivity = session->createTime;

    MediaSessionInfo* sessionPtr = session.get();
    sessions_[sessionId] = std::move(session);

    std::cout << "[MediaSessionManager] Created session: " << sessionId
              << " channel: " << channelId
              << " remote: " << remoteIp << std::endl;

    TriggerEvent(sessionId, SessionState::INVITING, "SESSION_CREATED");

    return sessionPtr;
}

MediaSessionInfo* MediaSessionManager::GetSession(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool MediaSessionManager::UpdateSessionState(const std::string& sessionId, SessionState state) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    SessionState oldState = it->second->state;
    it->second->state = state;
    it->second->lastActivity = std::chrono::system_clock::now();

    std::cout << "[MediaSessionManager] Session " << sessionId
              << " state: " << GetStateName(oldState)
              << " -> " << GetStateName(state) << std::endl;

    TriggerEvent(sessionId, state, "STATE_CHANGED");

    return true;
}

bool MediaSessionManager::SetLocalPorts(const std::string& sessionId,
                                        int localVideoPort,
                                        int localAudioPort) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    it->second->localVideoPort = localVideoPort;
    it->second->localAudioPort = localAudioPort;
    it->second->lastActivity = std::chrono::system_clock::now();

    std::cout << "[MediaSessionManager] Session " << sessionId
              << " local ports: video=" << localVideoPort
              << " audio=" << localAudioPort << std::endl;

    return true;
}

bool MediaSessionManager::SetRemotePorts(const std::string& sessionId,
                                         int remoteVideoPort,
                                         int remoteAudioPort) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    it->second->remoteVideoPort = remoteVideoPort;
    it->second->remoteAudioPort = remoteAudioPort;
    it->second->lastActivity = std::chrono::system_clock::now();

    std::cout << "[MediaSessionManager] Session " << sessionId
              << " remote ports: video=" << remoteVideoPort
              << " audio=" << remoteAudioPort << std::endl;

    return true;
}

bool MediaSessionManager::SetSsrc(const std::string& sessionId,
                                  uint32_t videoSsrc,
                                  uint32_t audioSsrc) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    it->second->videoSsrc = videoSsrc;
    it->second->audioSsrc = audioSsrc;
    it->second->lastActivity = std::chrono::system_clock::now();

    return true;
}

bool MediaSessionManager::TerminateSession(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        std::cerr << "[MediaSessionManager] Session not found: " << sessionId << std::endl;
        return false;
    }

    std::cout << "[MediaSessionManager] Terminating session: " << sessionId << std::endl;

    UpdateSessionState(sessionId, SessionState::TERMINATING);
    TriggerEvent(sessionId, SessionState::TERMINATED, "SESSION_TERMINATED");

    sessions_.erase(it);

    return true;
}

std::vector<std::string> MediaSessionManager::GetActiveSessions() {
    std::vector<std::string> activeSessions;

    for (const auto& pair : sessions_) {
        if (pair.second->state == SessionState::ESTABLISHED ||
            pair.second->state == SessionState::INVITING) {
            activeSessions.push_back(pair.first);
        }
    }

    return activeSessions;
}

size_t MediaSessionManager::GetSessionCount() {
    return sessions_.size();
}

size_t MediaSessionManager::CleanupTimeoutSessions(int timeoutSeconds) {
    auto now = std::chrono::system_clock::now();
    auto timeoutDuration = std::chrono::seconds(timeoutSeconds);
    size_t cleanupCount = 0;

    std::vector<std::string> timeoutSessions;

    for (const auto& pair : sessions_) {
        auto elapsed = now - pair.second->lastActivity;
        if (elapsed > timeoutDuration) {
            timeoutSessions.push_back(pair.first);
        }
    }

    for (const auto& sessionId : timeoutSessions) {
        std::cout << "[MediaSessionManager] Cleaning up timeout session: " << sessionId << std::endl;
        sessions_.erase(sessionId);
        cleanupCount++;
    }

    if (cleanupCount > 0) {
        std::cout << "[MediaSessionManager] Cleaned up " << cleanupCount << " timeout sessions" << std::endl;
    }

    return cleanupCount;
}

void MediaSessionManager::SetEventCallback(SessionEventCallback callback) {
    eventCallback_ = callback;
}

void MediaSessionManager::UpdateActivity(const std::string& sessionId) {
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        it->second->lastActivity = std::chrono::system_clock::now();
    }
}

std::string MediaSessionManager::GetStateName(SessionState state) {
    switch (state) {
        case SessionState::IDLE: return "IDLE";
        case SessionState::INVITING: return "INVITING";
        case SessionState::ESTABLISHED: return "ESTABLISHED";
        case SessionState::TERMINATING: return "TERMINATING";
        case SessionState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

uint32_t MediaSessionManager::GenerateSsrc() {
    // 生成随机SSRC
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(1, 0xFFFFFFFF);

    uint32_t ssrc;
    do {
        ssrc = dis(gen);
    } while (ssrc == 0); // SSRC不能为0

    return ssrc;
}

void MediaSessionManager::TriggerEvent(const std::string& sessionId,
                                       SessionState state,
                                       const std::string& event) {
    if (eventCallback_) {
        eventCallback_(sessionId, state, event);
    }
}

} // namespace gb28181
