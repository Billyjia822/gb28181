#ifndef EXOSIP_H
#define EXOSIP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file eXosip.h
 * @brief eXoSIP协议栈头文件
 *
 * 这是一个简化版的eXoSIP协议栈头文件，用于GB28181设备端
 * 实际项目中建议使用完整的libeXosip2库
 */

#include "osip_parser.h"
#include <stdint.h>

/* eXosip事件类型 */
typedef enum {
    EXOSIP_REGISTRATION_NEW,
    EXOSIP_REGISTRATION_SUCCESS,
    EXOSIP_REGISTRATION_FAILURE,
    EXOSIP_CALL_INVITE,
    EXOSIP_CALL_RINGING,
    EXOSIP_CALL_ANSWERED,
    EXOSIP_CALL_PROCEEDING,
    EXOSIP_CALL_REJECTED,
    EXOSIP_CALL_REQUESTFAILURE,
    EXOSIP_CALL_SERVERFAILURE,
    EXOSIP_CALL_GLOBALFAILURE,
    EXOSIP_CALL_CLOSED,
    EXOSIP_CALL_BYE,
    EXOSIP_CALL_ACK,
    EXOSIP_MESSAGE_NEW,
    EXOSIP_MESSAGE_SUCCESS,
    EXOSIP_MESSAGE_FAILURE,
    EXOSIP_SUBSCRIPTION_UPDATE,
    EXOSIP_SUBSCRIPTION_NOTIFY,
    EXOSIP_NOTIFICATION_NOANSWER,
    EXOSIP_IN_SUBSCRIPTION_NEW,
    EXOSIP_IN_SUBSCRIPTION_RELEASED,
    EXOSIP_EVENT_COUNT
} eXosip_event_type_t;

/* eXosip事件结构 */
typedef struct eXosip_event {
    eXosip_event_type_t type;
    int tid;                    /* 事务ID */
    int did;                    /* 对话ID */
    int cid;                    /* 呼叫ID */
    int sid;                    /* 订阅ID */
    int nid;                    /* 通知ID */
    osip_message_t *request;    /* SIP请求 */
    osip_message_t *response;   /* SIP响应 */
    void *ctx;                  /* 用户上下文 */
} eXosip_event_t;

/* eXosip上下文结构 */
typedef struct eXosip_t eXosip_t;

/* 初始化eXosip */
eXosip_t* eXosip_malloc(void);

/* 释放eXosip */
void eXosip_free(eXosip_t *excontext);

/* 初始化 */
int eXosip_init(eXosip_t *excontext);

/* 启动 */
int eXosip_listen_addr(eXosip_t *excontext, int protocol,
                       const char *addr, int port, int family);

/* 停止 */
void eXosip_quit(eXosip_t *excontext);

/* 锁定 */
void eXosip_lock(eXosip_t *excontext);

/* 解锁 */
void eXosip_unlock(eXosip_t *excontext);

/* 发送REGISTER */
int eXosip_register_init(eXosip_t *excontext, const char *from,
                         const char *proxy, const char *contact);

int eXosip_register_build_initial_register(eXosip_t *excontext, const char *from,
                                           const char *proxy, const char *contact,
                                           int expires, osip_message_t **reg);

int eXosip_register_send_register(eXosip_t *excontext, int rid, osip_message_t *reg);

int eXosip_register_send_unregister(eXosip_t *excontext, int rid, osip_message_t *reg);

int eXosip_register_build_register(eXosip_t *excontext, int rid, int expires, osip_message_t **reg);

/* 发送INVITE */
int eXosip_call_build_initial_invite(eXosip_t *excontext, osip_message_t **invite,
                                     const char *to, const char *from,
                                     const char *route, const char *subject);

int eXosip_call_send_initial_invite(eXosip_t *excontext, osip_message_t *invite);

/* 发送BYE */
int eXosip_call_build_request(eXosip_t *excontext, int did, const char *method,
                              osip_message_t **req);

int eXosip_call_send_request(eXosip_t *excontext, int did, osip_message_t *req);

/* 发送MESSAGE */
int eXosip_message_build_request(eXosip_t *excontext, osip_message_t **msg,
                                 const char *method, const char *to,
                                 const char *from, const char *route);

int eXosip_message_send_request(eXosip_t *excontext, osip_message_t *msg);

/* 发送MESSAGE响应 */
int eXosip_message_send_answer(eXosip_t *excontext, int tid, int status,
                               osip_message_t *answer);

/* 发送CALL响应 */
int eXosip_call_build_answer_and_send(eXosip_t *excontext, int tid, int status);

/* 构建CALL响应 */
int eXosip_call_build_answer2(eXosip_t *excontext, int tid, int status, osip_message_t **answer);

/* 发送响应 */
int eXosip_message_build_answer(eXosip_t *excontext, int tid, int status,
                                osip_message_t **answer);

int eXosip_call_send_answer(eXosip_t *excontext, int tid, int status,
                           osip_message_t *answer);

int eXosip_message_build_answer_and_send(eXosip_t *excontext, int tid, int status);

/* 获取事件 */
eXosip_event_t* eXosip_event_wait(eXosip_t *excontext, int tv_sec, int tv_usec);

/* 释放事件 */
void eXosip_event_free(eXosip_event_t *je);

/* 设置用户代理 */
void eXosip_set_user_agent(eXosip_t *excontext, const char *user_agent);

/* 自动重传 */
void eXosip_set_option(eXosip_t *excontext, int opt, void *value);

/* 获取公网地址 */
int eXosip_guess_localip(eXosip_t *excontext, int family, char *ip, int ip_size);

#ifdef __cplusplus
}
#endif

#endif /* EXOSIP_H */
