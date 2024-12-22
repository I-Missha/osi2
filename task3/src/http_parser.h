#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__
#include "llhttp.h"
#include "vec.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ParseState {
    NotParsed,
    ReqParsed,
    RespParsed,
} ParseState;

typedef struct Parser_results_t {
    ParseState parseState;
    llhttp_method_t method;
    uint8_t minor_version;
    uint8_t major_version;
    char *full_msg;
    char *url;
} Parser_res;

int parse_http_request(llhttp_t *parser, const char *data, int data_len);
void destroy_request_parser(llhttp_t *parser, Parser_res *p_res);
int init_request_parser(llhttp_t *parser, Parser_res *p_res);
int init_parser_res_structures(llhttp_t *parser, Parser_res *con_req);
#endif // !__HTTP_PARSER__
