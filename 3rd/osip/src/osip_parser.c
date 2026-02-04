#include "osip_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

osip_message_t* osip_message_new(void) {
    osip_message_t *msg = (osip_message_t*)malloc(sizeof(osip_message_t));
    if (msg) {
        memset(msg, 0, sizeof(osip_message_t));
        msg->type = OSIP_REQUEST;
    }
    return msg;
}

void osip_message_free(osip_message_t *msg) {
    if (msg) {
        if (msg->sip_method) free(msg->sip_method);
        if (msg->sip_uri) free(msg->sip_uri);
        if (msg->reason_phrase) free(msg->reason_phrase);
        if (msg->call_id) free(msg->call_id);
        if (msg->cseq) free(msg->cseq);
        if (msg->from) free(msg->from);
        if (msg->to) free(msg->to);
        if (msg->via) free(msg->via);
        if (msg->contact) free(msg->contact);
        if (msg->max_forwards) free(msg->max_forwards);
        if (msg->user_agent) free(msg->user_agent);
        if (msg->content_type) free(msg->content_type);
        if (msg->content_length) free(msg->content_length);
        if (msg->body) free(msg->body);
        free(msg);
    }
}

static char* trim(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t' || *str == '\r') str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r')) end--;
    *(end + 1) = '\0';
    return str;
}

int osip_message_parse(osip_message_t *msg, const char *buf, size_t len) {
    char *line = NULL;
    char *pos = (char*)buf;
    char *end = (char*)buf + len;
    char *colon = NULL;
    char *header_name = NULL;
    char *header_value = NULL;

    if (!msg || !buf || len == 0) return -1;

    /* 解析第一行 */
    line = pos;
    while (pos < end && *pos != '\r' && *pos != '\n') pos++;

    if (*pos == '\r' && *(pos + 1) == '\n') {
        *pos = '\0';
        pos += 2;
    } else if (*pos == '\n') {
        *pos = '\0';
        pos += 1;
    }

    /* 判断是请求还是响应 */
    if (strncmp(line, "SIP/2.0", 7) == 0) {
        msg->type = OSIP_RESPONSE;
        msg->status_code = atoi(line + 8);
    } else {
        msg->type = OSIP_REQUEST;
        char *space = strchr(line, ' ');
        if (space) {
            msg->sip_method = strndup(line, space - line);
            char *uri = space + 1;
            space = strchr(uri, ' ');
            if (space) {
                msg->sip_uri = strndup(uri, space - uri);
            }
        }
    }

    /* 解析头字段 */
    while (pos < end) {
        line = pos;
        while (pos < end && *pos != '\r' && *pos != '\n') pos++;

        if (pos == line) break;

        if (*pos == '\r' && *(pos + 1) == '\n') {
            *pos = '\0';
            pos += 2;
        } else if (*pos == '\n') {
            *pos = '\0';
            pos += 1;
        }

        colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            header_name = trim(line);
            header_value = trim(colon + 1);

            if (strcasecmp(header_name, "Call-ID") == 0 || strcasecmp(header_name, "i") == 0) {
                msg->call_id = strdup(header_value);
            } else if (strcasecmp(header_name, "CSeq") == 0) {
                msg->cseq = strdup(header_value);
            } else if (strcasecmp(header_name, "From") == 0 || strcasecmp(header_name, "f") == 0) {
                msg->from = strdup(header_value);
            } else if (strcasecmp(header_name, "To") == 0 || strcasecmp(header_name, "t") == 0) {
                msg->to = strdup(header_value);
            } else if (strcasecmp(header_name, "Via") == 0 || strcasecmp(header_name, "v") == 0) {
                msg->via = strdup(header_value);
            } else if (strcasecmp(header_name, "Contact") == 0 || strcasecmp(header_name, "m") == 0) {
                msg->contact = strdup(header_value);
            } else if (strcasecmp(header_name, "Max-Forwards") == 0) {
                msg->max_forwards = strdup(header_value);
            } else if (strcasecmp(header_name, "User-Agent") == 0) {
                msg->user_agent = strdup(header_value);
            } else if (strcasecmp(header_name, "Content-Type") == 0 ||
                       strcasecmp(header_name, "c") == 0) {
                msg->content_type = strdup(header_value);
            } else if (strcasecmp(header_name, "Content-Length") == 0 ||
                       strcasecmp(header_name, "l") == 0) {
                msg->content_length = strdup(header_value);
            }
        }
    }

    /* 解析消息体 */
    if (pos < end) {
        msg->body = strndup(pos, end - pos);
        msg->body_length = end - pos;
    }

    return 0;
}

int osip_message_to_str(const osip_message_t *msg, char **dest, size_t *len) {
    size_t total_len = 0;
    char *buf = NULL;
    char *p = NULL;

    if (!msg || !dest) return -1;

    /* 计算总长度 */
    if (msg->type == OSIP_REQUEST) {
        total_len += strlen(msg->sip_method) + strlen(msg->sip_uri) + 20;
    } else {
        total_len += 20;
    }

    if (msg->call_id) total_len += strlen(msg->call_id) + 20;
    if (msg->cseq) total_len += strlen(msg->cseq) + 10;
    if (msg->from) total_len += strlen(msg->from) + 10;
    if (msg->to) total_len += strlen(msg->to) + 8;
    if (msg->via) total_len += strlen(msg->via) + 8;
    if (msg->contact) total_len += strlen(msg->contact) + 12;
    if (msg->max_forwards) total_len += strlen(msg->max_forwards) + 18;
    if (msg->user_agent) total_len += strlen(msg->user_agent) + 16;
    if (msg->content_type) total_len += strlen(msg->content_type) + 18;
    if (msg->content_length) total_len += strlen(msg->content_length) + 20;
    if (msg->body) total_len += msg->body_length + 4;

    total_len += 256; /* 额外空间 */

    buf = (char*)malloc(total_len);
    if (!buf) return -1;

    p = buf;

    /* 构建请求行或状态行 */
    if (msg->type == OSIP_REQUEST) {
        p += sprintf(p, "%s %s SIP/2.0\r\n", msg->sip_method, msg->sip_uri);
    } else {
        p += sprintf(p, "SIP/2.0 %d %s\r\n", msg->status_code,
                    msg->reason_phrase ? msg->reason_phrase : "OK");
    }

    /* 添加头字段 */
    if (msg->call_id) p += sprintf(p, "Call-ID: %s\r\n", msg->call_id);
    if (msg->cseq) p += sprintf(p, "CSeq: %s\r\n", msg->cseq);
    if (msg->from) p += sprintf(p, "From: %s\r\n", msg->from);
    if (msg->to) p += sprintf(p, "To: %s\r\n", msg->to);
    if (msg->via) p += sprintf(p, "Via: %s\r\n", msg->via);
    if (msg->contact) p += sprintf(p, "Contact: %s\r\n", msg->contact);
    if (msg->max_forwards) p += sprintf(p, "Max-Forwards: %s\r\n", msg->max_forwards);
    if (msg->user_agent) p += sprintf(p, "User-Agent: %s\r\n", msg->user_agent);
    if (msg->content_type) p += sprintf(p, "Content-Type: %s\r\n", msg->content_type);
    if (msg->content_length) p += sprintf(p, "Content-Length: %s\r\n", msg->content_length);

    p += sprintf(p, "\r\n");

    /* 添加消息体 */
    if (msg->body) {
        memcpy(p, msg->body, msg->body_length);
        p += msg->body_length;
    }

    *p = '\0';

    *dest = buf;
    if (len) *len = p - buf;

    return 0;
}

void osip_message_set_header(osip_message_t *msg, const char *name, const char *value) {
    if (!msg || !name || !value) return;

    if (strcasecmp(name, "Call-ID") == 0 || strcasecmp(name, "i") == 0) {
        if (msg->call_id) free(msg->call_id);
        msg->call_id = strdup(value);
    } else if (strcasecmp(name, "CSeq") == 0) {
        if (msg->cseq) free(msg->cseq);
        msg->cseq = strdup(value);
    } else if (strcasecmp(name, "From") == 0 || strcasecmp(name, "f") == 0) {
        if (msg->from) free(msg->from);
        msg->from = strdup(value);
    } else if (strcasecmp(name, "To") == 0 || strcasecmp(name, "t") == 0) {
        if (msg->to) free(msg->to);
        msg->to = strdup(value);
    } else if (strcasecmp(name, "Via") == 0 || strcasecmp(name, "v") == 0) {
        if (msg->via) free(msg->via);
        msg->via = strdup(value);
    } else if (strcasecmp(name, "Contact") == 0 || strcasecmp(name, "m") == 0) {
        if (msg->contact) free(msg->contact);
        msg->contact = strdup(value);
    } else if (strcasecmp(name, "Max-Forwards") == 0) {
        if (msg->max_forwards) free(msg->max_forwards);
        msg->max_forwards = strdup(value);
    } else if (strcasecmp(name, "User-Agent") == 0) {
        if (msg->user_agent) free(msg->user_agent);
        msg->user_agent = strdup(value);
    } else if (strcasecmp(name, "Content-Type") == 0 || strcasecmp(name, "c") == 0) {
        if (msg->content_type) free(msg->content_type);
        msg->content_type = strdup(value);
    } else if (strcasecmp(name, "Content-Length") == 0 || strcasecmp(name, "l") == 0) {
        if (msg->content_length) free(msg->content_length);
        msg->content_length = strdup(value);
    }
}

const char* osip_message_get_header(const osip_message_t *msg, const char *name) {
    if (!msg || !name) return NULL;

    if (strcasecmp(name, "Call-ID") == 0 || strcasecmp(name, "i") == 0) {
        return msg->call_id;
    } else if (strcasecmp(name, "CSeq") == 0) {
        return msg->cseq;
    } else if (strcasecmp(name, "From") == 0 || strcasecmp(name, "f") == 0) {
        return msg->from;
    } else if (strcasecmp(name, "To") == 0 || strcasecmp(name, "t") == 0) {
        return msg->to;
    } else if (strcasecmp(name, "Via") == 0 || strcasecmp(name, "v") == 0) {
        return msg->via;
    } else if (strcasecmp(name, "Contact") == 0 || strcasecmp(name, "m") == 0) {
        return msg->contact;
    } else if (strcasecmp(name, "Max-Forwards") == 0) {
        return msg->max_forwards;
    } else if (strcasecmp(name, "User-Agent") == 0) {
        return msg->user_agent;
    } else if (strcasecmp(name, "Content-Type") == 0 || strcasecmp(name, "c") == 0) {
        return msg->content_type;
    } else if (strcasecmp(name, "Content-Length") == 0 || strcasecmp(name, "l") == 0) {
        return msg->content_length;
    }

    return NULL;
}

void osip_message_set_body(osip_message_t *msg, const char *body, size_t len) {
    if (!msg) return;

    if (msg->body) free(msg->body);

    if (body && len > 0) {
        msg->body = (char*)malloc(len + 1);
        if (msg->body) {
            memcpy(msg->body, body, len);
            msg->body[len] = '\0';
            msg->body_length = len;
        }
    } else {
        msg->body = NULL;
        msg->body_length = 0;
    }
}

const char* osip_message_get_body(const osip_message_t *msg) {
    if (!msg) return NULL;
    return msg->body;
}

void osip_message_set_content_type(osip_message_t *msg, const char *content_type) {
    if (!msg || !content_type) return;
    if (msg->content_type) free(msg->content_type);
    msg->content_type = strdup(content_type);
}

int osip_message_header_get_byname(const osip_message_t *msg, const char *name, int pos, osip_header_t **dest) {
    if (!msg || !name || !dest) return -1;

    /* 简化版本只支持基本头字段的查找 */
    osip_header_t *header = (osip_header_t*)malloc(sizeof(osip_header_t));
    if (!header) return -1;

    header->hname = NULL;
    header->hvalue = NULL;
    header->next = NULL;

    if (pos == 0) {
        if (strcasecmp(name, "WWW-Authenticate") == 0 || strcasecmp(name, "Authorization") == 0) {
            /* 这些头字段在简化版本中不直接支持，返回空结果 */
            header->hname = strdup(name);
            header->hvalue = strdup("");
            *dest = header;
            return 0;
        }
    }

    free(header);
    return -1;
}
