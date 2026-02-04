#include "sip/sip_manager.h"
#include "sip/media_session.h"
#include "sip/sdp_negotiator.h"
#include "utils/md5.h"
#include "eXosip.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <random>
#include <regex>
#include <iomanip>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>

namespace gb28181 {

class SipManager::Impl {
public:
    Impl() : excontext_(nullptr), registered_(false), cseq_(1), sn_(1), rtpPortBase_(50000) {
        excontext_ = eXosip_malloc();
        if (excontext_) {
            eXosip_init(excontext_);
        }

        // 初始化媒体会话管理器
        mediaSessionManager_ = std::make_unique<MediaSessionManager>();
        mediaSessionManager_->Initialize();
    }

    ~Impl() {
        if (excontext_) {
            if (registered_) {
                Unregister();
            }
            eXosip_quit(excontext_);
            eXosip_free(excontext_);
            excontext_ = nullptr;
        }
    }

    bool Initialize(const std::string& localIp, int localPort,
                   const std::string& deviceId, const std::string& realm) {
        if (!excontext_) {
            return false;
        }

        localPort_ = localPort;
        deviceId_ = deviceId;
        realm_ = realm;

        // 如果传入的IP是空或"auto"，则自动获取本机IP
        if (localIp.empty() || localIp == "auto") {
            char autoIp[64];
            if (eXosip_guess_localip(excontext_, AF_INET, autoIp, sizeof(autoIp)) == 0) {
                localIp_ = autoIp;
                std::cout << "Auto-detected local IP: " << localIp_ << std::endl;
            } else {
                localIp_ = "0.0.0.0";
                std::cout << "Failed to auto-detect IP, using 0.0.0.0" << std::endl;
            }
        } else {
            localIp_ = localIp;
        }

        // 启动监听
        if (eXosip_listen_addr(excontext_, IPPROTO_UDP, localIp_.c_str(), localPort, AF_INET) != 0) {
            std::cerr << "Failed to listen on " << localIp_ << ":" << localPort << std::endl;
            return false;
        }

        // 设置User-Agent
        eXosip_set_user_agent(excontext_, "GB28181-Device/1.0");

        std::cout << "SIP initialized on " << localIp_ << ":" << localPort << std::endl;
        return true;
    }

    bool RegisterToServer(const std::string& serverIp, int serverPort,
                         const std::string& username, const std::string& password) {
        if (!excontext_) {
            return false;
        }

        serverIp_ = serverIp;
        serverPort_ = serverPort;
        username_ = username;
        password_ = password;

        // 构建from和contact地址
        std::string from = "sip:" + username_ + "@" + realm_;
        std::string proxy = "sip:" + serverIp_ + ":" + std::to_string(serverPort_);
        std::string contact = "sip:" + username_ + "@" + localIp_ + ":" + std::to_string(localPort_);

        // 初始化REGISTER
        int rid = eXosip_register_init(excontext_, from.c_str(), proxy.c_str(), contact.c_str());
        if (rid < 0) {
            std::cerr << "Failed to initialize REGISTER" << std::endl;
            return false;
        }

        // 构建REGISTER消息
        osip_message_t *reg = nullptr;
        if (eXosip_register_build_initial_register(excontext_, from.c_str(), proxy.c_str(),
                                                   contact.c_str(), 3600, &reg) != 0) {
            std::cerr << "Failed to build REGISTER message" << std::endl;
            return false;
        }

        // 添加Authorization头（第一次注册时可能为空，等待401响应后重新认证）
        std::string auth = "Digest username=\"" + username_ + "\",realm=\"" + realm_ +
                          "\",nonce=\"\",uri=\"sip:" + realm_ + "\",response=\"" +
                          password_ + "\",algorithm=MD5";
        osip_message_set_header(reg, "Authorization", auth.c_str());

        // 发送REGISTER
        if (eXosip_register_send_register(excontext_, rid, reg) != 0) {
            std::cerr << "Failed to send REGISTER" << std::endl;
            return false;
        }

        std::cout << "REGISTER sent to " << serverIp << ":" << serverPort << std::endl;
        return true;
    }

    bool Unregister() {
        if (!excontext_ || !registered_) {
            return false;
        }

        std::string from = "sip:" + username_ + "@" + realm_;
        std::string proxy = "sip:" + serverIp_ + ":" + std::to_string(serverPort_);
        std::string contact = "sip:" + username_ + "@" + localIp_ + ":" + std::to_string(localPort_);

        osip_message_t *reg = nullptr;
        if (eXosip_register_build_initial_register(excontext_, from.c_str(), proxy.c_str(),
                                                   contact.c_str(), 0, &reg) != 0) {
            return false;
        }

        eXosip_register_send_register(excontext_, 0, reg);

        registered_ = false;
        std::cout << "UNREGISTER sent" << std::endl;
        return true;
    }

    bool SendHeartbeat() {
        if (!excontext_ || !registered_) {
            return false;
        }

        // GB28181使用MESSAGE发送心跳
        std::string to = "sip:" + serverIp_ + ":" + std::to_string(serverPort_);
        std::string from = "sip:" + username_ + "@" + realm_;

        osip_message_t *msg = nullptr;
        if (eXosip_message_build_request(excontext_, &msg, "MESSAGE",
                                         to.c_str(), from.c_str(), nullptr) != 0) {
            return false;
        }

        // 构建心跳消息体（MANSCDP格式）
        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Notify>\r\n";
        ss << "<CmdType>Keepalive</CmdType>\r\n";
        ss << "<SN>" << sn_++ << "</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<Status>OK</Status>\r\n";
        ss << "</Notify>\r\n";

        std::string body = ss.str();
        osip_message_set_body(msg, body.c_str(), body.length());
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");

        if (eXosip_message_send_request(excontext_, msg) != 0) {
            return false;
        }

        std::cout << "Keepalive sent" << std::endl;
        return true;
    }

    void ProcessMessage() {
        if (!excontext_) {
            return;
        }

        eXosip_event_t *event = eXosip_event_wait(excontext_, 0, 100);

        if (event) {
            switch (event->type) {
                case EXOSIP_REGISTRATION_SUCCESS:
                    registered_ = true;
                    std::cout << "[SIP] Registration successful" << std::endl;
                    if (eventCallback_) {
                        eventCallback_("REGISTER_SUCCESS", "Device registered successfully");
                    }
                    break;

                case EXOSIP_REGISTRATION_FAILURE:
                    // 检查是否是401未授权响应
                    if (event->response && event->response->status_code == 401) {
                        std::cout << "[SIP] Received 401 Unauthorized, performing digest authentication" << std::endl;
                        Handle401Response(event);
                    } else {
                        registered_ = false;
                        std::cout << "[SIP] Registration failed" << std::endl;
                        if (eventCallback_) {
                            eventCallback_("REGISTER_FAILURE", "Registration failed");
                        }
                    }
                    break;

                case EXOSIP_MESSAGE_NEW:
                    std::cout << "[SIP] New MESSAGE received" << std::endl;
                    HandleMessage(event);
                    break;

                case EXOSIP_CALL_INVITE:
                    std::cout << "[SIP] INVITE received" << std::endl;
                    HandleInvite(event);
                    break;

                case EXOSIP_CALL_ACK:
                    std::cout << "[SIP] ACK received" << std::endl;
                    HandleAck(event);
                    break;

                case EXOSIP_CALL_CLOSED:
                    std::cout << "[SIP] Call closed" << std::endl;
                    HandleBye(event);
                    break;

                default:
                    break;
            }

            eXosip_event_free(event);
        }
    }

    void SetEventCallback(SipEventCallback callback) {
        eventCallback_ = callback;
    }

    void SetMediaSessionEventCallback(MediaSessionEventCallback callback) {
        mediaSessionEventCallback_ = callback;
    }

    std::string GetLocalIp() const { return localIp_; }
    int GetLocalPort() const { return localPort_; }
    std::string GetDeviceId() const { return deviceId_; }

    bool SendMessage(const std::string& to, const std::string& content) {
        if (!excontext_) {
            return false;
        }

        std::string from = "sip:" + username_ + "@" + realm_;

        osip_message_t *msg = nullptr;
        if (eXosip_message_build_request(excontext_, &msg, "MESSAGE",
                                         to.c_str(), from.c_str(), nullptr) != 0) {
            return false;
        }

        osip_message_set_body(msg, content.c_str(), content.length());
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");

        if (eXosip_message_send_request(excontext_, msg) != 0) {
            return false;
        }

        return true;
    }

    bool SendResponse(int tid, int statusCode, const std::string& reason) {
        if (!excontext_) {
            return false;
        }

        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, statusCode, &answer) != 0) {
            return false;
        }

        if (!reason.empty()) {
            osip_message_set_header(answer, "Reason", reason.c_str());
        }

        eXosip_message_send_answer(excontext_, tid, statusCode, answer);
        osip_message_free(answer);
        return true;
    }

    bool PtzControl(const std::string& channelId, int command, int speed) {
        std::cout << "[PTZ] Direction control - Channel: " << channelId
                  << ", Command: " << command << ", Speed: " << speed << std::endl;

        if (eventCallback_) {
            std::string cmdStr;
            switch(command) {
                case 0: cmdStr = "STOP"; break;
                case 1: cmdStr = "UP"; break;
                case 2: cmdStr = "DOWN"; break;
                case 3: cmdStr = "LEFT"; break;
                case 4: cmdStr = "RIGHT"; break;
                case 5: cmdStr = "UP_LEFT"; break;
                case 6: cmdStr = "DOWN_LEFT"; break;
                case 7: cmdStr = "UP_RIGHT"; break;
                case 8: cmdStr = "DOWN_RIGHT"; break;
                default: cmdStr = "UNKNOWN"; break;
            }
            eventCallback_("PTZ_CONTROL", cmdStr + " speed=" + std::to_string(speed));
        }

        return true;
    }

    bool PtzZoom(const std::string& channelId, int command, int speed) {
        std::cout << "[PTZ] Zoom control - Channel: " << channelId
                  << ", Command: " << command << ", Speed: " << speed << std::endl;

        if (eventCallback_) {
            std::string cmdStr;
            switch(command) {
                case 0: cmdStr = "ZOOM_STOP"; break;
                case 1: cmdStr = "ZOOM_IN"; break;
                case 2: cmdStr = "ZOOM_OUT"; break;
                default: cmdStr = "ZOOM_UNKNOWN"; break;
            }
            eventCallback_("PTZ_ZOOM", cmdStr + " speed=" + std::to_string(speed));
        }

        return true;
    }

    bool PtzPreset(const std::string& channelId, int command, int presetId) {
        std::cout << "[PTZ] Preset control - Channel: " << channelId
                  << ", Command: " << command << ", PresetID: " << presetId << std::endl;

        if (eventCallback_) {
            std::string cmdStr;
            switch(command) {
                case 0: cmdStr = "PRESET_DELETE"; break;
                case 1: cmdStr = "PRESET_CALL"; break;
                case 2: cmdStr = "PRESET_SET"; break;
                default: cmdStr = "PRESET_UNKNOWN"; break;
            }
            eventCallback_("PTZ_PRESET", cmdStr + " id=" + std::to_string(presetId));
        }

        return true;
    }

    MediaSessionManager* GetMediaSessionManager() {
        return mediaSessionManager_.get();
    }

    std::vector<std::string> GetActiveMediaSessions() {
        return mediaSessionManager_->GetActiveSessions();
    }

private:
    void Handle401Response(eXosip_event_t *event) {
        if (!event->response) {
            return;
        }

        // 解析WWW-Authenticate头
        const char *authHeader = osip_message_get_header(event->response, "WWW-Authenticate");

        if (!authHeader || strlen(authHeader) == 0) {
            std::cerr << "[SIP] No WWW-Authenticate header found" << std::endl;
            return;
        }

        std::cout << "[SIP] WWW-Authenticate: " << authHeader << std::endl;

        // 解析认证参数
        std::string nonce = ParseAuthenticateParam(authHeader, "nonce");
        std::string realm = ParseAuthenticateParam(authHeader, "realm");
        std::string algorithm = ParseAuthenticateParam(authHeader, "algorithm");
        std::string qop = ParseAuthenticateParam(authHeader, "qop");

        if (nonce.empty()) {
            std::cerr << "[SIP] No nonce found in WWW-Authenticate" << std::endl;
            return;
        }

        // 使用解析的realm或默认realm
        if (realm.empty()) {
            realm = realm_;
        }

        // 计算Digest响应
        std::string uri = "sip:" + realm_;
        std::string digestResponse = MD5::CalculateDigestResponse(
            "REGISTER", uri, username_, realm, password_, nonce, qop, "1"
        );

        // 构建新的REGISTER消息
        std::string from = "sip:" + username_ + "@" + realm_;
        std::string proxy = "sip:" + serverIp_ + ":" + std::to_string(serverPort_);
        std::string contact = "sip:" + username_ + "@" + localIp_ + ":" + std::to_string(localPort_);

        osip_message_t *reg = nullptr;
        if (eXosip_register_build_register(excontext_, event->tid, 3600, &reg) != 0) {
            std::cerr << "[SIP] Failed to build REGISTER for digest auth" << std::endl;
            return;
        }

        // 构建Authorization头
        std::stringstream auth;
        auth << "Digest username=\"" << username_ << "\""
             << ",realm=\"" << realm << "\""
             << ",nonce=\"" << nonce << "\""
             << ",uri=\"" << uri << "\""
             << ",response=\"" << digestResponse << "\""
             << ",algorithm=" << algorithm;

        if (!qop.empty()) {
            auth << ",qop=" << qop
                 << ",cnonce=\"0a4f113b\""
                 << ",nc=00000001";
        }

        osip_message_set_header(reg, "Authorization", auth.str().c_str());

        // 发送REGISTER
        if (eXosip_register_send_register(excontext_, event->tid, reg) != 0) {
            std::cerr << "[SIP] Failed to send REGISTER with digest auth" << std::endl;
            return;
        }

        std::cout << "[SIP] Sent REGISTER with digest authentication" << std::endl;
    }

    std::string ParseAuthenticateParam(const std::string& authHeader, const std::string& paramName) {
        std::regex paramRegex(paramName + R"(=([^,\s]+))");
        std::smatch match;

        if (std::regex_search(authHeader, match, paramRegex)) {
            std::string value = match[1].str();
            // 移除引号
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            return value;
        }

        return "";
    }

    void HandleMessage(eXosip_event_t *event) {
        if (event->request) {
            // 解析消息内容
            const char *body = osip_message_get_body(event->request);
            if (body) {
                std::cout << "[SIP] MESSAGE body: " << body << std::endl;

                // 解析MANSCDP命令
                if (strstr(body, "<CmdType>Catalog</CmdType>")) {
                    SendCatalogResponse(event->tid);
                } else if (strstr(body, "<CmdType>DeviceInfo</CmdType>")) {
                    SendDeviceInfoResponse(event->tid);
                } else if (strstr(body, "<CmdType>DeviceStatus</CmdType>")) {
                    SendDeviceStatusResponse(event->tid);
                } else if (strstr(body, "<CmdType>RecordInfo</CmdType>")) {
                    std::cout << "[SIP] RecordInfo command received" << std::endl;
                    SendRecordInfoResponse(event->tid);
                } else if (strstr(body, "<CmdType>DeviceControl</CmdType>")) {
                    HandleDeviceControl(event);
                }
            }

            // 发送200 OK响应
            eXosip_message_build_answer_and_send(excontext_, event->tid, 200);
        }
    }

    void HandleInvite(eXosip_event_t *event) {
        std::cout << "[SIP] Processing INVITE for video streaming" << std::endl;

        // 获取Call-ID
        std::string callId = "";
        if (event->request) {
            const char *callIdStr = osip_message_get_header(event->request, "Call-ID");
            if (callIdStr) {
                callId = callIdStr;
            }
        }

        if (callId.empty()) {
            std::cerr << "[SIP] No Call-ID in INVITE" << std::endl;
            eXosip_call_send_answer(excontext_, event->tid, 400, nullptr);
            return;
        }

        std::cout << "[SIP] Call-ID: " << callId << std::endl;

        // 解析SDP offer
        const char *sdpBody = osip_message_get_body(event->request);
        if (!sdpBody) {
            std::cerr << "[SIP] No SDP body in INVITE" << std::endl;
            eXosip_call_send_answer(excontext_, event->tid, 400, nullptr);
            return;
        }

        std::cout << "[SIP] SDP Offer:\n" << sdpBody << std::endl;

        // 解析SDP获取远程信息
        std::string remoteIp;
        int remoteVideoPort = 0;
        int remoteAudioPort = 0;
        std::string videoCodec = "H264";
        std::string audioCodec = "PCMA";

        ParseSdpOffer(sdpBody, remoteIp, remoteVideoPort, remoteAudioPort, videoCodec, audioCodec);

        std::cout << "[SIP] Remote - IP: " << remoteIp
                  << ", VideoPort: " << remoteVideoPort
                  << ", AudioPort: " << remoteAudioPort
                  << ", VideoCodec: " << videoCodec
                  << ", AudioCodec: " << audioCodec << std::endl;

        // 分配本地RTP端口
        int localVideoPort = AllocateRtpPort();
        int localAudioPort = localVideoPort + 2;

        std::cout << "[SIP] Local - VideoPort: " << localVideoPort
                  << ", AudioPort: " << localAudioPort << std::endl;

        // 创建媒体会话
        MediaSessionInfo *session = mediaSessionManager_->CreateSession(
            callId, deviceId_, remoteIp, videoCodec, audioCodec
        );

        if (!session) {
            std::cerr << "[SIP] Failed to create media session" << std::endl;
            eXosip_call_send_answer(excontext_, event->tid, 500, nullptr);
            return;
        }

        // 设置会话端口
        mediaSessionManager_->SetLocalPorts(callId, localVideoPort, localAudioPort);
        mediaSessionManager_->SetRemotePorts(callId, remoteVideoPort, remoteAudioPort);

        // 生成SDP应答
        SdpNegotiator sdpNegotiator;
        std::string sdpAnswer = sdpNegotiator.CreateSdpAnswer(
            localIp_, localVideoPort,
            (videoCodec == "H264") ? SdpMediaFormat::H264 :
            (videoCodec == "H265") ? SdpMediaFormat::H265 : SdpMediaFormat::PS,
            (audioCodec == "PCMA") ? SdpMediaFormat::PCMA : SdpMediaFormat::PCMU
        );

        std::cout << "[SIP] SDP Answer:\n" << sdpAnswer << std::endl;

        // 发送200 OK响应
        osip_message_t *answer = nullptr;
        if (eXosip_call_build_answer2(excontext_, event->tid, 200, &answer) != 0) {
            std::cerr << "[SIP] Failed to build 200 OK answer" << std::endl;
            mediaSessionManager_->TerminateSession(callId);
            eXosip_call_send_answer(excontext_, event->tid, 500, nullptr);
            return;
        }

        // 设置SDP应答
        osip_message_set_body(answer, sdpAnswer.c_str(), sdpAnswer.length());
        osip_message_set_content_type(answer, "application/sdp");

        // 发送响应
        if (eXosip_call_send_answer(excontext_, event->tid, 200, answer) != 0) {
            std::cerr << "[SIP] Failed to send 200 OK answer" << std::endl;
            mediaSessionManager_->TerminateSession(callId);
            osip_message_free(answer);
            return;
        }

        // 更新会话状态
        mediaSessionManager_->UpdateSessionState(callId, SessionState::ESTABLISHED);

        std::cout << "[SIP] Sent 200 OK with SDP answer" << std::endl;

        if (eventCallback_) {
            eventCallback_("INVITE_ACCEPTED", "Video streaming session established");
        }

        if (mediaSessionEventCallback_) {
            mediaSessionEventCallback_(callId, "ESTABLISHED", "SESSION_ESTABLISHED");
        }
    }

    void HandleAck(eXosip_event_t *event) {
        std::cout << "[SIP] ACK received, session confirmed" << std::endl;

        // 获取Call-ID
        std::string callId = "";
        if (event->request) {
            const char *callIdStr = osip_message_get_header(event->request, "Call-ID");
            if (callIdStr) {
                callId = callIdStr;
            }
        }

        if (!callId.empty()) {
            std::cout << "[SIP] ACK for Call-ID: " << callId << std::endl;
            // 更新会话活动时间
            mediaSessionManager_->UpdateActivity(callId);

            if (eventCallback_) {
                eventCallback_("ACK_RECEIVED", "Video streaming started");
            }

            if (mediaSessionEventCallback_) {
                mediaSessionEventCallback_(callId, "ESTABLISHED", "STREAMING_STARTED");
            }
        }
    }

    void HandleBye(eXosip_event_t *event) {
        std::cout << "[SIP] Processing BYE to stop video streaming" << std::endl;

        // 获取Call-ID
        std::string callId = "";
        if (event->request) {
            const char *callIdStr = osip_message_get_header(event->request, "Call-ID");
            if (callIdStr) {
                callId = callIdStr;
            }
        }

        if (!callId.empty()) {
            std::cout << "[SIP] BYE for Call-ID: " << callId << std::endl;

            // 终止媒体会话
            mediaSessionManager_->TerminateSession(callId);

            if (eventCallback_) {
                eventCallback_("BYE_RECEIVED", "Video streaming stopped");
            }

            if (mediaSessionEventCallback_) {
                mediaSessionEventCallback_(callId, "TERMINATED", "STREAMING_STOPPED");
            }
        }

        // 发送200 OK响应
        eXosip_call_build_answer_and_send(excontext_, event->tid, 200);
    }

    void HandleDeviceControl(eXosip_event_t *event) {
        if (!event->request) {
            return;
        }

        const char *body = osip_message_get_body(event->request);
        if (!body) {
            eXosip_message_build_answer_and_send(excontext_, event->tid, 400);
            return;
        }

        // 解析PTZ命令
        const char *ptzCmd = strstr(body, "<PTZCmd>");
        if (ptzCmd) {
            char cmdStr[256] = {0};
            const char *start = strstr(ptzCmd, ">");
            if (start) {
                start++;
                const char *end = strstr(start, "<");
                if (end) {
                    int len = end - start;
                    if (len < sizeof(cmdStr)) {
                        strncpy(cmdStr, start, len);
                        cmdStr[len] = '\0';
                        std::cout << "[SIP] PTZ Command: " << cmdStr << std::endl;

                        int command = 0;
                        int speed = 128;
                        int presetId = 0;

                        if (strstr(cmdStr, "Command=1")) command = 1;
                        else if (strstr(cmdStr, "Command=2")) command = 2;
                        else if (strstr(cmdStr, "Command=3")) command = 3;
                        else if (strstr(cmdStr, "Command=4")) command = 4;
                        else if (strstr(cmdStr, "Command=5")) command = 5;
                        else if (strstr(cmdStr, "Command=6")) command = 6;
                        else if (strstr(cmdStr, "Command=7")) command = 7;
                        else if (strstr(cmdStr, "Command=8")) command = 8;
                        else if (strstr(cmdStr, "Command=11")) command = 11;
                        else if (strstr(cmdStr, "Command=12")) command = 12;
                        else if (strstr(cmdStr, "Command=21")) command = 21;
                        else if (strstr(cmdStr, "Command=22")) command = 22;
                        else if (strstr(cmdStr, "Command=23")) command = 23;

                        const char *speedStr = strstr(cmdStr, "Speed=");
                        if (speedStr) {
                            speed = atoi(speedStr + 6);
                        }

                        const char *presetStr = strstr(cmdStr, "PresetID=");
                        if (presetStr) {
                            presetId = atoi(presetStr + 9);
                        }

                        bool success = false;
                        if (command >= 1 && command <= 8) {
                            success = PtzControl(deviceId_, command, speed);
                        } else if (command == 11 || command == 12) {
                            int zoomCmd = (command == 11) ? 1 : 2;
                            success = PtzZoom(deviceId_, zoomCmd, speed);
                        } else if (command >= 21 && command <= 23) {
                            int presetCmd = (command == 21) ? 1 : ((command == 22) ? 2 : 0);
                            success = PtzPreset(deviceId_, presetCmd, presetId);
                        }

                        if (success) {
                            SendDeviceControlResponse(event->tid, "OK");
                        } else {
                            SendDeviceControlResponse(event->tid, "ERROR");
                        }
                        return;
                    }
                }
            }
        }

        eXosip_message_build_answer_and_send(excontext_, event->tid, 400);
    }

    void ParseSdpOffer(const std::string& sdpStr,
                      std::string& remoteIp,
                      int& remoteVideoPort,
                      int& remoteAudioPort,
                      std::string& videoCodec,
                      std::string& audioCodec) {
        std::istringstream ss(sdpStr);
        std::string line;

        std::string currentMedia;
        int currentPort = 0;

        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();

            if (line.length() < 2) continue;

            char type = line[0];
            std::string content = line.substr(2);

            switch (type) {
                case 'c': // 连接信息
                    if (content.find("IN IP4") != std::string::npos) {
                        size_t pos = content.find_last_of(' ');
                        if (pos != std::string::npos) {
                            remoteIp = content.substr(pos + 1);
                        }
                    }
                    break;

                case 'm': // 媒体描述
                    if (content.find("video") == 0) {
                        currentMedia = "video";
                        std::istringstream mss(content);
                        std::string dummy;
                        mss >> dummy >> currentPort;
                        remoteVideoPort = currentPort;
                    } else if (content.find("audio") == 0) {
                        currentMedia = "audio";
                        std::istringstream mss(content);
                        std::string dummy;
                        mss >> dummy >> currentPort;
                        remoteAudioPort = currentPort;
                    }
                    break;

                case 'a': // 属性
                    if (content.find("rtpmap:") == 0) {
                        std::string rtpmapContent = content.substr(7);
                        if (currentMedia == "video") {
                            if (rtpmapContent.find("H264") != std::string::npos) {
                                videoCodec = "H264";
                            } else if (rtpmapContent.find("H265") != std::string::npos) {
                                videoCodec = "H265";
                            } else if (rtpmapContent.find("PS") != std::string::npos) {
                                videoCodec = "PS";
                            }
                        } else if (currentMedia == "audio") {
                            if (rtpmapContent.find("PCMA") != std::string::npos) {
                                audioCodec = "PCMA";
                            } else if (rtpmapContent.find("PCMU") != std::string::npos) {
                                audioCodec = "PCMU";
                            }
                        }
                    }
                    break;
            }
        }
    }

    int AllocateRtpPort() {
        static int nextPort = rtpPortBase_;
        int port = nextPort;
        nextPort += 4; // 每个会话使用4个端口（视频2个，音频2个）
        return port;
    }

    bool SendCatalogResponse(int tid) {
        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, 200, &answer) != 0) {
            return false;
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>Catalog</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<SumNum>1</SumNum>\r\n";
        ss << "<DeviceList Num=\"1\">\r\n";
        ss << "<Item>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<Name>Camera 1</Name>\r\n";
        ss << "<Manufacturer>GB28181 Inc.</Manufacturer>\r\n";
        ss << "<Model>IPC-1000</Model>\r\n";
        ss << "<Status>ON</Status>\r\n";
        ss << "<IPAddress>" << localIp_ << "</IPAddress>\r\n";
        ss << "<Port>" << localPort_ << "</Port>\r\n";
        ss << "</Item>\r\n";
        ss << "</DeviceList>\r\n";
        ss << "</Response>\r\n";

        std::string body = ss.str();
        osip_message_set_body(answer, body.c_str(), body.length());
        osip_message_set_content_type(answer, "Application/MANSCDP+xml");

        eXosip_message_send_answer(excontext_, tid, 200, answer);
        osip_message_free(answer);
        return true;
    }

    bool SendDeviceInfoResponse(int tid) {
        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, 200, &answer) != 0) {
            return false;
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>DeviceInfo</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<DeviceName>GB28181 Camera</DeviceName>\r\n";
        ss << "<Manufacturer>GB28181 Inc.</Manufacturer>\r\n";
        ss << "<Model>IPC-1000</Model>\r\n";
        ss << "<FirmwareVersion>1.0.0</FirmwareVersion>\r\n";
        ss << "</Response>\r\n";

        std::string body = ss.str();
        osip_message_set_body(answer, body.c_str(), body.length());
        osip_message_set_content_type(answer, "Application/MANSCDP+xml");

        eXosip_message_send_answer(excontext_, tid, 200, answer);
        osip_message_free(answer);
        return true;
    }

    bool SendDeviceStatusResponse(int tid) {
        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, 200, &answer) != 0) {
            return false;
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>DeviceStatus</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<Result>OK</Result>\r\n";
        ss << "<Online>ONLINE</Online>\r\n";
        ss << "<Status>OK</Status>\r\n";
        ss << "<Encode>ON</Encode>\r\n";
        ss << "<Record>OFF</Record>\r\n";
        ss << "</Response>\r\n";

        std::string body = ss.str();
        osip_message_set_body(answer, body.c_str(), body.length());
        osip_message_set_content_type(answer, "Application/MANSCDP+xml");

        eXosip_message_send_answer(excontext_, tid, 200, answer);
        osip_message_free(answer);
        return true;
    }

    bool SendRecordInfoResponse(int tid) {
        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, 200, &answer) != 0) {
            return false;
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>RecordInfo</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<SumNum>0</SumNum>\r\n";
        ss << "<RecordList Num=\"0\">\r\n";
        ss << "</RecordList>\r\n";
        ss << "</Response>\r\n";

        std::string body = ss.str();
        osip_message_set_body(answer, body.c_str(), body.length());
        osip_message_set_content_type(answer, "Application/MANSCDP+xml");

        eXosip_message_send_answer(excontext_, tid, 200, answer);
        osip_message_free(answer);
        return true;
    }

    bool SendDeviceControlResponse(int tid, const std::string& result) {
        osip_message_t *answer = nullptr;
        if (eXosip_message_build_answer(excontext_, tid, 200, &answer) != 0) {
            return false;
        }

        std::stringstream ss;
        ss << "<?xml version=\"1.0\"?>\r\n";
        ss << "<Response>\r\n";
        ss << "<CmdType>DeviceControl</CmdType>\r\n";
        ss << "<SN>1</SN>\r\n";
        ss << "<DeviceID>" << deviceId_ << "</DeviceID>\r\n";
        ss << "<Result>" << result << "</Result>\r\n";
        ss << "</Response>\r\n";

        std::string body = ss.str();
        osip_message_set_body(answer, body.c_str(), body.length());
        osip_message_set_content_type(answer, "Application/MANSCDP+xml");

        eXosip_message_send_answer(excontext_, tid, 200, answer);
        osip_message_free(answer);
        return true;
    }

private:
    eXosip_t *excontext_;
    std::string localIp_;
    int localPort_;
    std::string deviceId_;
    std::string realm_;
    std::string serverIp_;
    int serverPort_;
    std::string username_;
    std::string password_;
    bool registered_;
    int cseq_;
    int sn_;
    SipEventCallback eventCallback_;
    MediaSessionEventCallback mediaSessionEventCallback_;
    std::unique_ptr<MediaSessionManager> mediaSessionManager_;
    int rtpPortBase_;
};

SipManager::SipManager() : impl_(new Impl()) {}

SipManager::~SipManager() {}

bool SipManager::Initialize(const std::string& localIp, int localPort,
                           const std::string& deviceId, const std::string& realm) {
    return impl_->Initialize(localIp, localPort, deviceId, realm);
}

bool SipManager::RegisterToServer(const std::string& serverIp, int serverPort,
                                 const std::string& username, const std::string& password) {
    return impl_->RegisterToServer(serverIp, serverPort, username, password);
}

bool SipManager::Unregister() {
    return impl_->Unregister();
}

bool SipManager::SendHeartbeat() {
    return impl_->SendHeartbeat();
}

void SipManager::ProcessMessage() {
    impl_->ProcessMessage();
}

void SipManager::SetEventCallback(SipEventCallback callback) {
    impl_->SetEventCallback(callback);
}

void SipManager::SetMediaSessionEventCallback(MediaSessionEventCallback callback) {
    impl_->SetMediaSessionEventCallback(callback);
}

std::string SipManager::GetLocalIp() const {
    return impl_->GetLocalIp();
}

int SipManager::GetLocalPort() const {
    return impl_->GetLocalPort();
}

std::string SipManager::GetDeviceId() const {
    return impl_->GetDeviceId();
}

bool SipManager::SendMessage(const std::string& to, const std::string& content) {
    return impl_->SendMessage(to, content);
}

bool SipManager::SendResponse(int tid, int statusCode, const std::string& reason) {
    return impl_->SendResponse(tid, statusCode, reason);
}

bool SipManager::PtzControl(const std::string& channelId, int command, int speed) {
    return impl_->PtzControl(channelId, command, speed);
}

bool SipManager::PtzZoom(const std::string& channelId, int command, int speed) {
    return impl_->PtzZoom(channelId, command, speed);
}

bool SipManager::PtzPreset(const std::string& channelId, int command, int presetId) {
    return impl_->PtzPreset(channelId, command, presetId);
}

MediaSessionManager* SipManager::GetMediaSessionManager() {
    return impl_->GetMediaSessionManager();
}

std::vector<std::string> SipManager::GetActiveMediaSessions() {
    return impl_->GetActiveMediaSessions();
}

} // namespace gb28181
