#ifndef GB28181_MEDIA_SESSION_H
#define GB28181_MEDIA_SESSION_H

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <chrono>

namespace gb28181 {

/**
 * @brief 媒体会话状态
 */
enum class SessionState {
    IDLE,           // 空闲
    INVITING,       // 邀请中
    ESTABLISHED,    // 已建立
    TERMINATING,    // 终止中
    TERMINATED      // 已终止
};

/**
 * @brief 媒体流类型
 */
enum class MediaType {
    VIDEO,
    AUDIO,
    VIDEO_AUDIO
};

/**
 * @brief 媒体会话信息
 */
struct MediaSessionInfo {
    std::string sessionId;       // 会话ID (Call-ID)
    std::string channelId;      // 通道ID
    std::string remoteIp;       // 远程IP
    int remoteVideoPort;        // 远程视频端口
    int remoteAudioPort;        // 远程音频端口
    std::string localIp;        // 本地IP
    int localVideoPort;         // 本地视频端口
    int localAudioPort;         // 本地音频端口
    MediaType mediaType;        // 媒体类型
    SessionState state;         // 会话状态
    std::string videoCodec;     // 视频编码格式
    std::string audioCodec;     // 音频编码格式
    uint32_t videoSsrc;         // 视频SSRC
    uint32_t audioSsrc;         // 音频SSRC
    std::chrono::system_clock::time_point createTime;  // 创建时间
    std::chrono::system_clock::time_point lastActivity; // 最后活动时间
};

/**
 * @brief 媒体会话事件回调
 */
using SessionEventCallback = std::function<void(const std::string& sessionId,
                                                SessionState state,
                                                const std::string& event)>;

/**
 * @brief 媒体会话管理器
 * 管理视频点播、回放等媒体会话
 */
class MediaSessionManager {
public:
    MediaSessionManager();
    ~MediaSessionManager();

    /**
     * @brief 初始化会话管理器
     * @return 是否成功
     */
    bool Initialize();

    /**
     * @brief 创建新的媒体会话
     * @param sessionId 会话ID (Call-ID)
     * @param channelId 通道ID
     * @param remoteIp 远程IP
     * @param videoCodec 视频编码格式
     * @param audioCodec 音频编码格式
     * @return 会话信息指针
     */
    MediaSessionInfo* CreateSession(const std::string& sessionId,
                                   const std::string& channelId,
                                   const std::string& remoteIp,
                                   const std::string& videoCodec = "H264",
                                   const std::string& audioCodec = "PCMA");

    /**
     * @brief 获取会话
     * @param sessionId 会话ID
     * @return 会话信息指针，不存在返回nullptr
     */
    MediaSessionInfo* GetSession(const std::string& sessionId);

    /**
     * @brief 更新会话状态
     * @param sessionId 会话ID
     * @param state 新状态
     * @return 是否成功
     */
    bool UpdateSessionState(const std::string& sessionId, SessionState state);

    /**
     * @brief 设置本地媒体端口
     * @param sessionId 会话ID
     * @param localVideoPort 本地视频端口
     * @param localAudioPort 本地音频端口
     * @return 是否成功
     */
    bool SetLocalPorts(const std::string& sessionId,
                      int localVideoPort,
                      int localAudioPort);

    /**
     * @brief 设置远程媒体端口
     * @param sessionId 会话ID
     * @param remoteVideoPort 远程视频端口
     * @param remoteAudioPort 远程音频端口
     * @return 是否成功
     */
    bool SetRemotePorts(const std::string& sessionId,
                       int remoteVideoPort,
                       int remoteAudioPort);

    /**
     * @brief 设置SSRC
     * @param sessionId 会话ID
     * @param videoSsrc 视频SSRC
     * @param audioSsrc 音频SSRC
     * @return 是否成功
     */
    bool SetSsrc(const std::string& sessionId,
                uint32_t videoSsrc,
                uint32_t audioSsrc);

    /**
     * @brief 终止会话
     * @param sessionId 会话ID
     * @return 是否成功
     */
    bool TerminateSession(const std::string& sessionId);

    /**
     * @brief 获取所有活跃会话
     * @return 会话ID列表
     */
    std::vector<std::string> GetActiveSessions();

    /**
     * @brief 获取会话数量
     * @return 会话数量
     */
    size_t GetSessionCount();

    /**
     * @brief 清理超时会话
     * @param timeoutSeconds 超时时间（秒）
     * @return 清理的会话数量
     */
    size_t CleanupTimeoutSessions(int timeoutSeconds = 300);

    /**
     * @brief 设置事件回调
     * @param callback 回调函数
     */
    void SetEventCallback(SessionEventCallback callback);

    /**
     * @brief 更新会话活动时间
     * @param sessionId 会话ID
     */
    void UpdateActivity(const std::string& sessionId);

    /**
     * @brief 获取状态名称
     */
    static std::string GetStateName(SessionState state);

private:
    /**
     * @brief 生成SSRC
     */
    uint32_t GenerateSsrc();

    /**
     * @brief 触发事件回调
     */
    void TriggerEvent(const std::string& sessionId, SessionState state, const std::string& event);

private:
    std::map<std::string, std::unique_ptr<MediaSessionInfo>> sessions_;
    SessionEventCallback eventCallback_;
    bool initialized_;
    uint32_t ssrcCounter_;
};

} // namespace gb28181

#endif // GB28181_MEDIA_SESSION_H
