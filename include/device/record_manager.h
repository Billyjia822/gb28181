#ifndef GB28181_RECORD_MANAGER_H
#define GB28181_RECORD_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace gb28181 {

/**
 * @brief 录像类型
 */
enum class RecordType {
    TIME,      // 定时录像
    MANUAL,    // 手动录像
    ALARM,     // 告警录像
    ALL        // 所有类型
};

/**
 * @brief 录像信息
 */
struct RecordInfo {
    std::string deviceId;       // 设备ID
    std::string channelId;      // 通道ID
    std::string startTime;      // 开始时间 (格式: 2024-01-01T00:00:00)
    std::string endTime;        // 结束时间 (格式: 2024-01-01T00:00:00)
    RecordType type;            // 录像类型
    std::string filePath;       // 文件路径
    uint64_t fileSize;          // 文件大小(字节)
    std::string storage;        // 存储位置
    bool hasPrivacy;            // 是否包含隐私遮挡
};

/**
 * @brief 录像查询条件
 */
struct RecordQueryCondition {
    std::string channelId;      // 通道ID
    std::string startTime;      // 开始时间
    std::string endTime;        // 结束时间
    RecordType type;            // 录像类型
    int maxResults;             // 最大返回结果数
    std::string order;          // 排序方式 (asc/desc)
};

/**
 * @brief 录像管理器
 * 用于GB28181录像查询功能
 */
class RecordManager {
public:
    RecordManager();
    ~RecordManager();

    /**
     * @brief 初始化录像管理器
     * @param recordPath 录像文件存储路径
     * @return 是否成功
     */
    bool Initialize(const std::string& recordPath);

    /**
     * @brief 查询录像信息
     * @param condition 查询条件
     * @return 录像信息列表
     */
    std::vector<RecordInfo> QueryRecords(const RecordQueryCondition& condition);

    /**
     * @brief 添加录像信息
     * @param record 录像信息
     */
    void AddRecord(const RecordInfo& record);

    /**
     * @brief 删除录像信息
     * @param deviceId 设备ID
     * @param channelId 通道ID
     * @param startTime 开始时间
     */
    void DeleteRecord(const std::string& deviceId,
                     const std::string& channelId,
                     const std::string& startTime);

    /**
     * @brief 生成录像查询响应
     * @param deviceId 设备ID
     * @param sn 序列号
     * @param sumNum 总数
     * @param records 录像列表
     * @return MANSCDP XML响应字符串
     */
    std::string GenerateRecordInfoResponse(const std::string& deviceId,
                                          const std::string& sn,
                                          int sumNum,
                                          const std::vector<RecordInfo>& records);

    /**
     * @brief 从存储路径加载录像信息
     */
    bool LoadRecordsFromStorage();

    /**
     * @brief 扫描录像文件
     */
    void ScanRecordFiles();

    /**
     * @brief 获取存储路径
     */
    std::string GetRecordPath() const { return recordPath_; }

private:
    /**
     * @brief 解析录像文件名
     */
    bool ParseRecordFileName(const std::string& fileName,
                            RecordInfo& record);

    /**
     * @brief 获取文件大小
     */
    uint64_t GetFileSize(const std::string& filePath);

private:
    std::string recordPath_;
    std::vector<RecordInfo> records_;
    bool initialized_;
};

} // namespace gb28181

#endif // GB28181_RECORD_MANAGER_H
