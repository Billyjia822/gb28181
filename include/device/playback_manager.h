#ifndef GB28181_PLAYBACK_MANAGER_H
#define GB28181_PLAYBACK_MANAGER_H

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>

namespace gb28181 {

/**
 * @brief 回放模式
 */
enum class PlaybackMode {
    NORMAL,    // 正常播放
    PAUSE,     // 暂停
    FORWARD,   // 快进
    BACKWARD,  // 快退
    STEP       // 逐帧
};

/**
 * @brief 回放控制命令
 */
struct PlaybackControl {
    PlaybackMode mode;      // 回放模式
    double speed;           // 播放速度 (1.0为正常)
    uint64_t position;      // 播放位置(毫秒)
    bool isAudio;           // 是否包含音频
};

/**
 * @brief 回放会话信息
 */
struct PlaybackSession {
    std::string sessionId;   // 会话ID
    std::string channelId;  // 通道ID
    std::string filePath;   // 文件路径
    uint64_t startTime;     // 开始时间(毫秒)
    uint64_t endTime;       // 结束时间(毫秒)
    uint64_t currentPosition; // 当前位置(毫秒)
    PlaybackControl control; // 控制参数
    bool isActive;          // 是否活跃
};

/**
 * @brief 回放管理器
 * 用于GB28181历史视音频回放功能
 */
class PlaybackManager {
public:
    PlaybackManager();
    ~PlaybackManager();

    /**
     * @brief 初始化回放管理器
     * @return 是否成功
     */
    bool Initialize();

    /**
     * @brief 开始回放
     * @param channelId 通道ID
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param filePath 文件路径
     * @return 会话ID
     */
    std::string StartPlayback(const std::string& channelId,
                             const std::string& startTime,
                             const std::string& endTime,
                             const std::string& filePath);

    /**
     * @brief 停止回放
     * @param sessionId 会话ID
     * @return 是否成功
     */
    bool StopPlayback(const std::string& sessionId);

    /**
     * @brief 控制回放
     * @param sessionId 会话ID
     * @param control 控制参数
     * @return 是否成功
     */
    bool ControlPlayback(const std::string& sessionId, const PlaybackControl& control);

    /**
     * @brief 暂停回放
     * @param sessionId 会话ID
     * @return 是否成功
     */
    bool PausePlayback(const std::string& sessionId);

    /**
     * @brief 恢复回放
     * @param sessionId 会话ID
     * @return 是否成功
     */
    bool ResumePlayback(const std::string& sessionId);

    /**
     * @brief 快进
     * @param sessionId 会话ID
     * @param speed 速度倍数
     * @return 是否成功
     */
    bool FastForward(const std::string& sessionId, double speed);

    /**
     * @brief 快退
     * @param sessionId 会话ID
     * @param speed 速度倍数
     * @return 是否成功
     */
    bool FastBackward(const std::string& sessionId, double speed);

    /**
     * @brief 拖动播放
     * @param sessionId 会话ID
     * @param position 播放位置(毫秒)
     * @return 是否成功
     */
    bool SeekPlayback(const std::string& sessionId, uint64_t position);

    /**
     * @brief 获取回放会话
     * @param sessionId 会话ID
     * @return 会话信息
     */
    PlaybackSession* GetSession(const std::string& sessionId);

    /**
     * @brief 获取所有活跃会话
     * @return 会话列表
     */
    std::vector<std::string> GetActiveSessions();

    /**
     * @brief 设置帧回调
     * @param callback 帧数据回调函数
     */
    void SetFrameCallback(std::function<void(const uint8_t* data, size_t len, uint64_t timestamp)> callback);

    /**
     * @brief 读取下一帧
     * @param sessionId 会话ID
     * @return 是否成功
     */
    bool ReadNextFrame(const std::string& sessionId);

    /**
     * @brief 检查文件是否存在
     * @param filePath 文件路径
     * @return 是否存在
     */
    bool FileExists(const std::string& filePath);

private:
    /**
     * @brief 生成会话ID
     */
    std::string GenerateSessionId();

    /**
     * @brief 解析时间字符串为毫秒
     */
    uint64_t ParseTimeToMs(const std::string& timeStr);

    /**
     * @brief 格式化毫秒为时间字符串
     */
    std::string FormatMsToTime(uint64_t ms);

private:
    std::map<std::string, std::unique_ptr<PlaybackSession>> sessions_;
    std::function<void(const uint8_t*, size_t, uint64_t)> frameCallback_;
    bool initialized_;
    int sessionCounter_;
};

} // namespace gb28181

#endif // GB28181_PLAYBACK_MANAGER_H
