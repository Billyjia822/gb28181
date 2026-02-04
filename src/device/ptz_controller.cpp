#include "device/ptz_controller.h"
#include <sstream>
#include <algorithm>
#include <regex>
#include <iostream>

namespace gb28181 {

PTZController::PTZController() : initialized_(false) {
}

PTZController::~PTZController() {
}

bool PTZController::Initialize() {
    initialized_ = true;
    std::cout << "[PTZController] Initialized" << std::endl;
    return true;
}

bool PTZController::ExecuteCommand(const PTZControlParams& params) {
    if (!initialized_) {
        std::cerr << "[PTZController] Not initialized" << std::endl;
        return false;
    }

    std::cout << "[PTZController] Execute command: " << GetCommandName(params.command)
              << " on channel: " << params.channelId
              << " speed: " << params.speed << std::endl;

    // 更新当前动作
    currentActions_[params.channelId] = params;

    // 执行硬件控制
    bool success = ExecuteHardwareControl(params);

    if (success) {
        std::cout << "[PTZController] Command executed successfully" << std::endl;
    } else {
        std::cerr << "[PTZController] Command execution failed" << std::endl;
    }

    return success;
}

bool PTZController::StopAll(const std::string& channelId) {
    PTZControlParams params;
    params.command = PTZCommand::STOP;
    params.speed = 0;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::SetPreset(const std::string& channelId, int presetId, const std::string& name) {
    // TODO: 从硬件获取当前位置信息
    PTZPreset preset;
    preset.presetId = presetId;
    preset.name = name.empty() ? ("Preset " + std::to_string(presetId)) : name;
    preset.pan = 0.0;
    preset.tilt = 0.0;
    preset.zoom = 1.0;
    preset.focus = 0.0;
    preset.valid = true;

    presets_[channelId][presetId] = preset;

    std::cout << "[PTZController] Set preset: " << presetId
              << " for channel: " << channelId << std::endl;

    return true;
}

bool PTZController::CallPreset(const std::string& channelId, int presetId, int speed) {
    auto it = presets_.find(channelId);
    if (it == presets_.end()) {
        std::cerr << "[PTZController] Channel not found: " << channelId << std::endl;
        return false;
    }

    auto presetIt = it->second.find(presetId);
    if (presetIt == it->second.end()) {
        std::cerr << "[PTZController] Preset not found: " << presetId << std::endl;
        return false;
    }

    PTZControlParams params;
    params.command = PTZCommand::PRESET_CALL;
    params.speed = speed;
    params.presetId = presetId;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::DeletePreset(const std::string& channelId, int presetId) {
    auto it = presets_.find(channelId);
    if (it == presets_.end()) {
        return false;
    }

    it->second.erase(presetId);

    std::cout << "[PTZController] Deleted preset: " << presetId << std::endl;

    return true;
}

PTZPreset PTZController::GetPreset(const std::string& channelId, int presetId) {
    auto it = presets_.find(channelId);
    if (it == presets_.end()) {
        return PTZPreset{presetId, "", 0.0, 0.0, 1.0, 0.0, false};
    }

    auto presetIt = it->second.find(presetId);
    if (presetIt == it->second.end()) {
        return PTZPreset{presetId, "", 0.0, 0.0, 1.0, 0.0, false};
    }

    return presetIt->second;
}

std::vector<PTZPreset> PTZController::GetAllPresets(const std::string& channelId) {
    std::vector<PTZPreset> result;

    auto it = presets_.find(channelId);
    if (it != presets_.end()) {
        for (const auto& pair : it->second) {
            result.push_back(pair.second);
        }
    }

    return result;
}

bool PTZController::AddCruisePoint(const std::string& channelId, int cruiseId, int presetId,
                                   int speed, int dwellTime) {
    auto& cruise = cruises_[channelId][cruiseId];
    cruise.cruiseId = cruiseId;
    cruise.name = "Cruise " + std::to_string(cruiseId);
    cruise.presetIds.push_back(presetId);
    cruise.speeds.push_back(speed);
    cruise.dwellTimes.push_back(dwellTime);
    cruise.enabled = true;

    std::cout << "[PTZController] Added cruise point: cruiseId=" << cruiseId
              << " presetId=" << presetId << std::endl;

    return true;
}

bool PTZController::DeleteCruisePoint(const std::string& channelId, int cruiseId, int presetId) {
    auto it = cruises_.find(channelId);
    if (it == cruises_.end()) {
        return false;
    }

    auto cruiseIt = it->second.find(cruiseId);
    if (cruiseIt == it->second.end()) {
        return false;
    }

    auto& cruise = cruiseIt->second;
    for (size_t i = 0; i < cruise.presetIds.size(); i++) {
        if (cruise.presetIds[i] == presetId) {
            cruise.presetIds.erase(cruise.presetIds.begin() + i);
            cruise.speeds.erase(cruise.speeds.begin() + i);
            cruise.dwellTimes.erase(cruise.dwellTimes.begin() + i);
            break;
        }
    }

    return true;
}

bool PTZController::StartCruise(const std::string& channelId, int cruiseId) {
    auto it = cruises_.find(channelId);
    if (it == cruises_.end()) {
        return false;
    }

    auto cruiseIt = it->second.find(cruiseId);
    if (cruiseIt == it->second.end() || !cruiseIt->second.enabled) {
        return false;
    }

    PTZControlParams params;
    params.command = PTZCommand::CRUISE_START;
    params.cruiseId = cruiseId;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::StopCruise(const std::string& channelId, int cruiseId) {
    PTZControlParams params;
    params.command = PTZCommand::CRUISE_STOP;
    params.cruiseId = cruiseId;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::StartScan(const std::string& channelId) {
    PTZControlParams params;
    params.command = PTZCommand::SCAN_START;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::StopScan(const std::string& channelId) {
    PTZControlParams params;
    params.command = PTZCommand::SCAN_STOP;
    params.channelId = channelId;

    return ExecuteCommand(params);
}

bool PTZController::ParsePTZCommand(const std::string& cmdStr, PTZControlParams& params) {
    // 清空参数
    params = PTZControlParams{};
    params.speed = 128; // 默认速度

    // 解析参数: Command=1&Speed=128&PresetID=1
    std::regex paramRegex(R"((\w+)=([^\s&]+))");
    std::sregex_iterator it(cmdStr.begin(), cmdStr.end(), paramRegex);
    std::sregex_iterator end;

    while (it != end) {
        std::string key = it->str(1);
        std::string value = it->str(2);

        if (key == "Command") {
            int code = std::stoi(value);
            params.command = ParseCommandCode(code);
        } else if (key == "Speed") {
            params.speed = std::stoi(value);
        } else if (key == "PresetID") {
            params.presetId = std::stoi(value);
        } else if (key == "DwellTime") {
            params.dwellTime = std::stoi(value);
        } else if (key == "CruiseID") {
            params.cruiseId = std::stoi(value);
        }

        ++it;
    }

    return true;
}

bool PTZController::ParsePTZCommandFromXML(const std::string& xmlStr, PTZControlParams& params) {
    // 使用XML解析器解析
    // 这里简化处理，实际应该使用完整的XML解析器

    // 查找PTZCmd节点内容
    size_t pos = xmlStr.find("<PTZCmd>");
    if (pos == std::string::npos) {
        return false;
    }

    pos += 8;
    size_t endPos = xmlStr.find("</PTZCmd>", pos);
    if (endPos == std::string::npos) {
        return false;
    }

    std::string cmdStr = xmlStr.substr(pos, endPos - pos);

    return ParsePTZCommand(cmdStr, params);
}

void PTZController::SetHardwareCallback(std::function<bool(const PTZControlParams&)> callback) {
    hardwareCallback_ = callback;
}

std::string PTZController::GetPTZStatus(const std::string& channelId) {
    std::stringstream ss;

    auto it = currentActions_.find(channelId);
    if (it != currentActions_.end()) {
        const auto& params = it->second;
        ss << "Command: " << GetCommandName(params.command) << "\n";
        ss << "Speed: " << params.speed << "\n";

        if (params.presetId > 0) {
            ss << "PresetID: " << params.presetId << "\n";
        }

        if (params.cruiseId > 0) {
            ss << "CruiseID: " << params.cruiseId << "\n";
        }
    } else {
        ss << "No active PTZ action";
    }

    return ss.str();
}

bool PTZController::ExecuteHardwareControl(const PTZControlParams& params) {
    // 如果设置了硬件回调，调用回调函数
    if (hardwareCallback_) {
        return hardwareCallback_(params);
    }

    // 否则模拟执行
    // TODO: 实际应该调用硬件驱动接口
    // 例如: 通过串口发送PELCO-D/PELCO-P命令
    // 或者通过网络发送VISCA命令

    std::cout << "[PTZController] Hardware control: " << GetCommandName(params.command) << std::endl;

    return true;
}

PTZCommand PTZController::ParseCommandCode(int code) {
    // GB28181 PTZ命令编码
    switch (code) {
        case 0:  return PTZCommand::STOP;
        case 1:  return PTZCommand::UP;
        case 2:  return PTZCommand::DOWN;
        case 3:  return PTZCommand::LEFT;
        case 4:  return PTZCommand::RIGHT;
        case 5:  return PTZCommand::UP_LEFT;
        case 6:  return PTZCommand::DOWN_LEFT;
        case 7:  return PTZCommand::UP_RIGHT;
        case 8:  return PTZCommand::DOWN_RIGHT;
        case 11: return PTZCommand::ZOOM_IN;
        case 12: return PTZCommand::ZOOM_OUT;
        case 13: return PTZCommand::FOCUS_NEAR;
        case 14: return PTZCommand::FOCUS_FAR;
        case 15: return PTZCommand::IRIS_OPEN;
        case 16: return PTZCommand::IRIS_CLOSE;
        case 21: return PTZCommand::PRESET_CALL;
        case 22: return PTZCommand::PRESET_SET;
        case 23: return PTZCommand::PRESET_DELETE;
        case 31: return PTZCommand::CRUISE_START;
        case 32: return PTZCommand::CRUISE_STOP;
        case 33: return PTZCommand::CRUISE_ADD;
        case 41: return PTZCommand::SCAN_START;
        case 42: return PTZCommand::SCAN_STOP;
        default: return PTZCommand::STOP;
    }
}

std::string PTZController::GetCommandName(PTZCommand command) {
    switch (command) {
        case PTZCommand::STOP:           return "STOP";
        case PTZCommand::UP:             return "UP";
        case PTZCommand::DOWN:           return "DOWN";
        case PTZCommand::LEFT:           return "LEFT";
        case PTZCommand::RIGHT:          return "RIGHT";
        case PTZCommand::UP_LEFT:        return "UP_LEFT";
        case PTZCommand::UP_RIGHT:       return "UP_RIGHT";
        case PTZCommand::DOWN_LEFT:      return "DOWN_LEFT";
        case PTZCommand::DOWN_RIGHT:     return "DOWN_RIGHT";
        case PTZCommand::ZOOM_IN:        return "ZOOM_IN";
        case PTZCommand::ZOOM_OUT:       return "ZOOM_OUT";
        case PTZCommand::FOCUS_NEAR:     return "FOCUS_NEAR";
        case PTZCommand::FOCUS_FAR:      return "FOCUS_FAR";
        case PTZCommand::IRIS_OPEN:      return "IRIS_OPEN";
        case PTZCommand::IRIS_CLOSE:     return "IRIS_CLOSE";
        case PTZCommand::PRESET_SET:     return "PRESET_SET";
        case PTZCommand::PRESET_CALL:    return "PRESET_CALL";
        case PTZCommand::PRESET_DELETE:  return "PRESET_DELETE";
        case PTZCommand::CRUISE_START:   return "CRUISE_START";
        case PTZCommand::CRUISE_STOP:    return "CRUISE_STOP";
        case PTZCommand::CRUISE_ADD:     return "CRUISE_ADD";
        case PTZCommand::SCAN_START:     return "SCAN_START";
        case PTZCommand::SCAN_STOP:      return "SCAN_STOP";
        default:                          return "UNKNOWN";
    }
}

} // namespace gb28181
