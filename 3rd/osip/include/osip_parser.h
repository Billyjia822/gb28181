#ifndef OSIP_PARSER_H
#define OSIP_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file osip_parser.h
 * @brief oSIP协议解析器头文件
 *
 * 这是一个简化版的oSIP协议栈头文件，用于GB28181设备端
 * 实际项目中建议使用完整的osip2库
 */

#include <stdint.h>
#include <stddef.h>

/* SIP方法类型 */
typedef enum {
    OSIP_INVITE,
    OSIP_ACK,
    OSIP_OPTIONS,
    OSIP_BYE,
    OSIP_CANCEL,
    OSIP_REGISTER,
    OSIP_PRACK,
    OSIP_INFO,
    OSIP_MESSAGE,
    OSIP_SUBSCRIBE,
    OSIP_NOTIFY,
    OSIP_UNKNOWN
} osip_method_t;

/* SIP消息类型 */
typedef enum {
    OSIP_REQUEST,
    OSIP_RESPONSE
} osip_message_type_t;

/* Call-ID 结构 */
typedef struct osip_call_id {
    char *number;
    char *host;
} osip_call_id_t;

/* Header 结构 */
typedef struct osip_header {
    char *hname;
    char *hvalue;
    struct osip_header *next;
} osip_header_t;

/* SIP消息结构 */
typedef struct osip_message {
    osip_message_type_t type;
    char *sip_method;
    char *sip_uri;
    int status_code;
    char *reason_phrase;

    /* 头字段 */
    union {
        char *call_id;  /* 简化版本使用char* */
        osip_call_id_t *call_id_struct;  /* 完整版本使用osip_call_id_t* */
    };
    char *cseq;
    char *from;
    char *to;
    char *via;
    char *contact;
    char *max_forwards;
    char *user_agent;
    char *content_type;
    char *content_length;

    /* 消息体 */
    char *body;
    size_t body_length;
} osip_message_t;

/* 创建SIP消息 */
osip_message_t* osip_message_new(void);

/* 释放SIP消息 */
void osip_message_free(osip_message_t *msg);

/* 解析SIP消息 */
int osip_message_parse(osip_message_t *msg, const char *buf, size_t len);

/* 序列化SIP消息 */
int osip_message_to_str(const osip_message_t *msg, char **dest, size_t *len);

/* 设置头字段 */
void osip_message_set_header(osip_message_t *msg, const char *name, const char *value);

/* 获取头字段 */
const char* osip_message_get_header(const osip_message_t *msg, const char *name);

/* 设置消息体 */
void osip_message_set_body(osip_message_t *msg, const char *body, size_t len);

/* 获取消息体 */
const char* osip_message_get_body(const osip_message_t *msg);

/* 设置Content-Type */
void osip_message_set_content_type(osip_message_t *msg, const char *content_type);

/* 获取头字段（按名称） */
int osip_message_header_get_byname(const osip_message_t *msg, const char *name, int pos, osip_header_t **dest);

#ifdef __cplusplus
}
#endif

#endif /* OSIP_PARSER_H */
