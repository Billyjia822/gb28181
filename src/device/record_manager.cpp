#include "device/record_manager.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <sys/stat.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace gb28181 {

RecordManager::RecordManager() : initialized_(false) {
}

RecordManager::~RecordManager() {
}

bool RecordManager::Initialize(const std::string& recordPath) {
    recordPath_ = recordPath;

    // 检查目录是否存在
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(recordPath_.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        // 尝试创建目录
        if (!CreateDirectoryA(recordPath_.c_str(), NULL)) {
            std::cerr << "[RecordManager] Failed to create directory: " << recordPath_ << std::endl;
            return false;
        }
    }
#else
    DIR* dir = opendir(recordPath_.c_str());
    if (!dir) {
        if (mkdir(recordPath_.c_str(), 0755) != 0) {
            std::cerr << "[RecordManager] Failed to create directory: " << recordPath_ << std::endl;
            return false;
        }
    } else {
        closedir(dir);
    }
#endif

    initialized_ = true;
    std::cout << "[RecordManager] Initialized with path: " << recordPath_ << std::endl;

    // 加载录像信息
    LoadRecordsFromStorage();

    return true;
}

std::vector<RecordInfo> RecordManager::QueryRecords(const RecordQueryCondition& condition) {
    std::vector<RecordInfo> results;

    if (!initialized_) {
        std::cerr << "[RecordManager] Not initialized" << std::endl;
        return results;
    }

    for (const auto& record : records_) {
        // 过滤通道
        if (!condition.channelId.empty() && record.channelId != condition.channelId) {
            continue;
        }

        // 过滤录像类型
        if (condition.type != RecordType::ALL && record.type != condition.type) {
            continue;
        }

        // 过滤时间范围
        if (!condition.startTime.empty() && record.endTime < condition.startTime) {
            continue;
        }
        if (!condition.endTime.empty() && record.startTime > condition.endTime) {
            continue;
        }

        results.push_back(record);

        // 限制结果数量
        if (condition.maxResults > 0 && results.size() >= condition.maxResults) {
            break;
        }
    }

    // 排序
    if (condition.order == "asc") {
        std::sort(results.begin(), results.end(),
            [](const RecordInfo& a, const RecordInfo& b) {
                return a.startTime < b.startTime;
            });
    } else if (condition.order == "desc") {
        std::sort(results.begin(), results.end(),
            [](const RecordInfo& a, const RecordInfo& b) {
                return a.startTime > b.startTime;
            });
    }

    std::cout << "[RecordManager] Query returned " << results.size() << " records" << std::endl;

    return results;
}

void RecordManager::AddRecord(const RecordInfo& record) {
    records_.push_back(record);

    std::cout << "[RecordManager] Added record: " << record.channelId
              << " from " << record.startTime << " to " << record.endTime << std::endl;
}

void RecordManager::DeleteRecord(const std::string& deviceId,
                                 const std::string& channelId,
                                 const std::string& startTime) {
    auto it = std::remove_if(records_.begin(), records_.end(),
        [&deviceId, &channelId, &startTime](const RecordInfo& record) {
            return record.deviceId == deviceId &&
                   record.channelId == channelId &&
                   record.startTime == startTime;
        });

    if (it != records_.end()) {
        records_.erase(it, records_.end());
        std::cout << "[RecordManager] Deleted record: " << channelId
                  << " at " << startTime << std::endl;
    }
}

std::string RecordManager::GenerateRecordInfoResponse(const std::string& deviceId,
                                                     const std::string& sn,
                                                     int sumNum,
                                                     const std::vector<RecordInfo>& records) {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>RecordInfo</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << deviceId << "</DeviceID>\r\n";
    ss << "<SumNum>" << sumNum << "</SumNum>\r\n";
    ss << "<RecordList Num=\"" << records.size() << "\">\r\n";

    for (const auto& record : records) {
        ss << "<Item>\r\n";
        ss << "<DeviceID>" << record.deviceId << "</DeviceID>\r\n";
        ss << "<ChannelID>" << record.channelId << "</ChannelID>\r\n";
        ss << "<StartTime>" << record.startTime << "</StartTime>\r\n";
        ss << "<EndTime>" << record.endTime << "</EndTime>\r\n";

        // 录像类型
        std::string typeStr;
        switch (record.type) {
            case RecordType::TIME: typeStr = "time"; break;
            case RecordType::MANUAL: typeStr = "manual"; break;
            case RecordType::ALARM: typeStr = "alarm"; break;
            default: typeStr = "time"; break;
        }
        ss << "<RecordType>" << typeStr << "</RecordType>\r\n";

        ss << "<FilePath>" << record.filePath << "</FilePath>\r\n";
        ss << "<FileSize>" << record.fileSize << "</FileSize>\r\n";
        ss << "</Item>\r\n";
    }

    ss << "</RecordList>\r\n";
    ss << "</Response>\r\n";

    return ss.str();
}

bool RecordManager::LoadRecordsFromStorage() {
    if (!initialized_) {
        return false;
    }

    ScanRecordFiles();

    std::cout << "[RecordManager] Loaded " << records_.size() << " records from storage" << std::endl;

    return true;
}

void RecordManager::ScanRecordFiles() {
    if (!initialized_) {
        return;
    }

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = recordPath_ + "\\*.mp4";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fileName = findData.cFileName;
            RecordInfo record;
            if (ParseRecordFileName(fileName, record)) {
                record.filePath = recordPath_ + "\\" + fileName;
                record.fileSize = GetFileSize(record.filePath);
                AddRecord(record);
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
    }
#else
    DIR* dir = opendir(recordPath_.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            if (fileName.find(".mp4") != std::string::npos ||
                fileName.find(".avi") != std::string::npos) {
                RecordInfo record;
                if (ParseRecordFileName(fileName, record)) {
                    record.filePath = recordPath_ + "/" + fileName;
                    record.fileSize = GetFileSize(record.filePath);
                    AddRecord(record);
                }
            }
        }
        closedir(dir);
    }
#endif
}

bool RecordManager::ParseRecordFileName(const std::string& fileName, RecordInfo& record) {
    // 文件名格式示例: 34020000001320000001_20240101_120000_20240101_130000.mp4
    // 或者: ChannelID_StartTime_EndTime.mp4

    std::regex pattern(R"((\d{20})_(\d{8}_\d{6})_(\d{8}_\d{6})\.(mp4|avi))");
    std::smatch match;

    if (std::regex_match(fileName, match, pattern)) {
        record.channelId = match[1].str();
        record.startTime = match[2].str();
        record.endTime = match[3].str();
        record.type = RecordType::TIME;
        record.hasPrivacy = false;
        return true;
    }

    return false;
}

uint64_t RecordManager::GetFileSize(const std::string& filePath) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        LARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        return size.QuadPart;
    }
#else
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0) {
        return fileStat.st_size;
    }
#endif

    return 0;
}

} // namespace gb28181
