#include "eXosip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define closesocket close
#endif

/* 事件队列 */
#define MAX_EVENTS 100

struct eXosip_t {
    int initialized;
    int socket;
    char local_ip[64];
    int local_port;
    char user_agent[128];

    eXosip_event_t *events[MAX_EVENTS];
    int event_count;
    int event_read_index;
    int event_write_index;
};

eXosip_t* eXosip_malloc(void) {
    eXosip_t *excontext = (eXosip_t*)malloc(sizeof(eXosip_t));
    if (excontext) {
        memset(excontext, 0, sizeof(eXosip_t));
        strcpy(excontext->user_agent, "eXosip/0.0.0");
    }
    return excontext;
}

void eXosip_free(eXosip_t *excontext) {
    if (excontext) {
        eXosip_quit(excontext);
        free(excontext);
    }
}

int eXosip_init(eXosip_t *excontext) {
    if (!excontext) return -1;

    excontext->initialized = 1;
    excontext->socket = -1;
    excontext->event_count = 0;
    excontext->event_read_index = 0;
    excontext->event_write_index = 0;

    return 0;
}

int eXosip_listen_addr(eXosip_t *excontext, int protocol,
                       const char *addr, int port, int family) {
    if (!excontext || !excontext->initialized) return -1;

    strncpy(excontext->local_ip, addr, sizeof(excontext->local_ip) - 1);
    excontext->local_port = port;

    /* 创建UDP socket */
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    excontext->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    excontext->socket = socket(AF_INET, SOCK_DGRAM, 0);
#endif

    if (excontext->socket < 0) {
        return -1;
    }

    /* 绑定地址 */
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(addr);
    local_addr.sin_port = htons(port);

    if (bind(excontext->socket, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
#ifdef _WIN32
        closesocket(excontext->socket);
#else
        close(excontext->socket);
#endif
        excontext->socket = -1;
        return -1;
    }

    return 0;
}

void eXosip_quit(eXosip_t *excontext) {
    if (!excontext) return;

    /* 释放所有事件 */
    while (excontext->event_count > 0) {
        eXosip_event_t *event = excontext->events[excontext->event_read_index];
        if (event) {
            eXosip_event_free(event);
        }
        excontext->event_read_index = (excontext->event_read_index + 1) % MAX_EVENTS;
        excontext->event_count--;
    }

    /* 关闭socket */
    if (excontext->socket >= 0) {
#ifdef _WIN32
        closesocket(excontext->socket);
        WSACleanup();
#else
        close(excontext->socket);
#endif
        excontext->socket = -1;
    }

    excontext->initialized = 0;
}

void eXosip_lock(eXosip_t *excontext) {
    /* 简化实现，实际应该使用互斥锁 */
}

void eXosip_unlock(eXosip_t *excontext) {
    /* 简化实现，实际应该使用互斥锁 */
}

int eXosip_register_init(eXosip_t *excontext, const char *from,
                         const char *proxy, const char *contact) {
    /* 简化实现 */
    return 0;
}

int eXosip_register_build_initial_register(eXosip_t *excontext, const char *from,
                                           const char *proxy, const char *contact,
                                           int expires, osip_message_t **reg) {
    if (!excontext || !reg) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_REQUEST;
    msg->sip_method = strdup("REGISTER");
    msg->sip_uri = strdup(proxy);

    osip_message_set_header(msg, "To", from);
    osip_message_set_header(msg, "From", from);
    osip_message_set_header(msg, "Call-ID", "123456789@192.168.1.100");
    osip_message_set_header(msg, "CSeq", "1 REGISTER");
    osip_message_set_header(msg, "Via", "SIP/2.0/UDP 192.168.1.100:5060;rport;branch=z9hG4bK123456");
    osip_message_set_header(msg, "Max-Forwards", "70");
    osip_message_set_header(msg, "Contact", contact);
    osip_message_set_header(msg, "User-Agent", excontext->user_agent);
    osip_message_set_header(msg, "Expires", "3600");

    *reg = msg;
    return 0;
}

int eXosip_register_send_register(eXosip_t *excontext, int rid, osip_message_t *reg) {
    if (!excontext || !reg || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(reg, &msg_str, &msg_len) != 0) {
        return -1;
    }

    /* 发送到SIP服务器 */
    /* 这里需要解析To头获取服务器地址 */
    /* 简化实现，假设已经知道服务器地址 */

    free(msg_str);
    return 0;
}

int eXosip_register_send_unregister(eXosip_t *excontext, int rid, osip_message_t *reg) {
    return eXosip_register_send_register(excontext, rid, reg);
}

int eXosip_register_build_register(eXosip_t *excontext, int rid, int expires, osip_message_t **reg) {
    if (!excontext || !reg) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_REQUEST;
    msg->sip_method = strdup("REGISTER");

    char expires_str[16];
    snprintf(expires_str, sizeof(expires_str), "%d", expires);
    osip_message_set_header(msg, "Expires", expires_str);

    *reg = msg;
    return 0;
}

int eXosip_call_build_initial_invite(eXosip_t *excontext, osip_message_t **invite,
                                     const char *to, const char *from,
                                     const char *route, const char *subject) {
    if (!excontext || !invite) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_REQUEST;
    msg->sip_method = strdup("INVITE");
    msg->sip_uri = strdup(to);

    osip_message_set_header(msg, "To", to);
    osip_message_set_header(msg, "From", from);
    osip_message_set_header(msg, "Call-ID", "123456789@192.168.1.100");
    osip_message_set_header(msg, "CSeq", "1 INVITE");
    osip_message_set_header(msg, "Via", "SIP/2.0/UDP 192.168.1.100:5060;rport;branch=z9hG4bK123456");
    osip_message_set_header(msg, "Max-Forwards", "70");
    osip_message_set_header(msg, "Contact", "<sip:192.168.1.100:5060>");
    osip_message_set_header(msg, "Content-Type", "application/sdp");
    osip_message_set_header(msg, "User-Agent", excontext->user_agent);
    osip_message_set_header(msg, "Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE, MESSAGE, INFO, NOTIFY, REFER");

    *invite = msg;
    return 0;
}

int eXosip_call_send_initial_invite(eXosip_t *excontext, osip_message_t *invite) {
    if (!excontext || !invite || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(invite, &msg_str, &msg_len) != 0) {
        return -1;
    }

    /* 发送INVITE */
    /* 简化实现 */

    free(msg_str);
    return 0;
}

int eXosip_call_build_request(eXosip_t *excontext, int did, const char *method,
                              osip_message_t **req) {
    if (!excontext || !req) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_REQUEST;
    msg->sip_method = strdup(method);

    *req = msg;
    return 0;
}

int eXosip_call_send_request(eXosip_t *excontext, int did, osip_message_t *req) {
    if (!excontext || !req || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(req, &msg_str, &msg_len) != 0) {
        return -1;
    }

    free(msg_str);
    return 0;
}

int eXosip_message_build_request(eXosip_t *excontext, osip_message_t **msg,
                                 const char *method, const char *to,
                                 const char *from, const char *route) {
    if (!excontext || !msg) return -1;

    osip_message_t *message = osip_message_new();
    if (!message) return -1;

    message->type = OSIP_REQUEST;
    message->sip_method = strdup(method);
    message->sip_uri = strdup(to);

    osip_message_set_header(message, "To", to);
    osip_message_set_header(message, "From", from);
    osip_message_set_header(message, "Call-ID", "123456789@192.168.1.100");
    osip_message_set_header(message, "CSeq", "1 MESSAGE");
    osip_message_set_header(message, "Via", "SIP/2.0/UDP 192.168.1.100:5060;rport;branch=z9hG4bK123456");
    osip_message_set_header(message, "Max-Forwards", "70");
    osip_message_set_header(message, "Content-Type", "Application/MANSCDP+xml");
    osip_message_set_header(message, "User-Agent", excontext->user_agent);

    *msg = message;
    return 0;
}

int eXosip_message_send_request(eXosip_t *excontext, osip_message_t *msg) {
    if (!excontext || !msg || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(msg, &msg_str, &msg_len) != 0) {
        return -1;
    }

    free(msg_str);
    return 0;
}

int eXosip_message_build_answer(eXosip_t *excontext, int tid, int status,
                                osip_message_t **answer) {
    if (!excontext || !answer) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_RESPONSE;
    msg->status_code = status;

    char status_str[16];
    snprintf(status_str, sizeof(status_str), "%d", status);
    osip_message_set_header(msg, "CSeq", status_str);
    osip_message_set_header(msg, "Via", "SIP/2.0/UDP 192.168.1.100:5060;rport;branch=z9hG4bK123456");
    osip_message_set_header(msg, "From", "<sip:34020000001320000001@3402000000>;tag=123456");
    osip_message_set_header(msg, "To", "<sip:34020000001320000001@3402000000>");
    osip_message_set_header(msg, "Call-ID", "123456789@192.168.1.100");
    osip_message_set_header(msg, "User-Agent", excontext->user_agent);

    *answer = msg;
    return 0;
}

int eXosip_call_send_answer(eXosip_t *excontext, int tid, int status,
                           osip_message_t *answer) {
    if (!excontext || !answer || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(answer, &msg_str, &msg_len) != 0) {
        return -1;
    }

    free(msg_str);
    return 0;
}

int eXosip_message_build_answer_and_send(eXosip_t *excontext, int tid, int status) {
    osip_message_t *answer = NULL;
    int ret = eXosip_message_build_answer(excontext, tid, status, &answer);
    if (ret != 0 || !answer) {
        return ret;
    }

    ret = eXosip_message_send_answer(excontext, tid, status, answer);
    osip_message_free(answer);
    return ret;
}

int eXosip_call_build_answer_and_send(eXosip_t *excontext, int tid, int status) {
    osip_message_t *answer = NULL;
    int ret = eXosip_message_build_answer(excontext, tid, status, &answer);
    if (ret != 0 || !answer) {
        return ret;
    }

    ret = eXosip_call_send_answer(excontext, tid, status, answer);
    osip_message_free(answer);
    return ret;
}

int eXosip_call_build_answer2(eXosip_t *excontext, int tid, int status, osip_message_t **answer) {
    if (!excontext || !answer) return -1;

    osip_message_t *msg = osip_message_new();
    if (!msg) return -1;

    msg->type = OSIP_RESPONSE;
    msg->status_code = status;

    char status_str[16];
    snprintf(status_str, sizeof(status_str), "%d", status);
    osip_message_set_header(msg, "CSeq", status_str);
    osip_message_set_header(msg, "Via", "SIP/2.0/UDP 192.168.1.100:5060;rport;branch=z9hG4bK123456");
    osip_message_set_header(msg, "From", "<sip:34020000001320000001@3402000000>;tag=123456");
    osip_message_set_header(msg, "To", "<sip:34020000001320000001@3402000000>");
    osip_message_set_header(msg, "Call-ID", "123456789@192.168.1.100");
    osip_message_set_header(msg, "User-Agent", excontext->user_agent);
    osip_message_set_header(msg, "Content-Type", "application/sdp");

    *answer = msg;
    return 0;
}

int eXosip_message_send_answer(eXosip_t *excontext, int tid, int status,
                               osip_message_t *answer) {
    if (!excontext || !answer || excontext->socket < 0) return -1;

    char *msg_str = NULL;
    size_t msg_len = 0;

    if (osip_message_to_str(answer, &msg_str, &msg_len) != 0) {
        return -1;
    }

    /* 发送响应消息 */
    free(msg_str);
    return 0;
}

eXosip_event_t* eXosip_event_wait(eXosip_t *excontext, int tv_sec, int tv_usec) {
    if (!excontext) return NULL;

    if (excontext->event_count > 0) {
        eXosip_event_t *event = excontext->events[excontext->event_read_index];
        excontext->event_read_index = (excontext->event_read_index + 1) % MAX_EVENTS;
        excontext->event_count--;
        return event;
    }

    return NULL;
}

void eXosip_event_free(eXosip_event_t *je) {
    if (je) {
        if (je->request) osip_message_free(je->request);
        if (je->response) osip_message_free(je->response);
        free(je);
    }
}

void eXosip_set_user_agent(eXosip_t *excontext, const char *user_agent) {
    if (excontext && user_agent) {
        strncpy(excontext->user_agent, user_agent, sizeof(excontext->user_agent) - 1);
    }
}

void eXosip_set_option(eXosip_t *excontext, int opt, void *value) {
    /* 简化实现 */
}

int eXosip_guess_localip(eXosip_t *excontext, int family, char *ip, int ip_size) {
    if (!excontext || !ip) return -1;

    strncpy(ip, excontext->local_ip, ip_size - 1);
    ip[ip_size - 1] = '\0';
    return 0;
}
