#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/time.h>

typedef struct 
{
    unsigned short id;
    unsigned short flags;
    unsigned short qd_count;
    unsigned short an_count;
    unsigned short ns_count;
    unsigned short ar_count;

}DNS_HEADER;

typedef struct 
{
    unsigned short QT;
    unsigned short QC;

}QUESTION;

#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

typedef struct 
{
    unsigned char* name;
    QUESTION *ques;
}QUERY;

typedef struct 
{
    char** black_list;
    int bl_tokens;
    char* upstream_dns_ip;
    char* response;
}config;