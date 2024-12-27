#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__
#include <llhttp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vec.h>

#define PARSE_ERROR 2
#define SUCCESS_PARSE 0
#define BUFFER_SIZE 4096 * 10

typedef enum ParseState {
    NotParsed,
    MsgParsed,
    UrlParsed,
    MethodParsed,
    VersionParsed,
} ParseState;

typedef struct ParserResults_t {
    ParseState parseStateMsg;
    ParseState parseStateUrl;
    ParseState parseStateMethod;
    ParseState parseStateVersion;
    llhttp_method_t method;
    uint8_t minor_version;
    uint8_t major_version;
    char **full_msg;
    char **url;
} Parser_res;

int init_parser(
    llhttp_t *parser,
    Parser_res *p_res,
    llhttp_type_t http_type,
    char **full_msg,
    char **url
);

int is_method_acceptable(Parser_res *p_res);
int is_version_acceptable(Parser_res *p_res);
void vector_push_str(char **vec, char *str, int str_size);
int receive_parsed_request(int client_fd, llhttp_t *parser, Parser_res *p_res);
void http_parse_host_name(const char *url, char **host_name);
int parse_http(llhttp_t *parser, const char *data, int data_len);
void destroy_parser(llhttp_t *parser, Parser_res *p_res);
int init_parser_res_structures(llhttp_t *parser, Parser_res *con_req);
#endif // !__HTTP_PARSER__
