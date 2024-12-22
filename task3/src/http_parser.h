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

typedef struct parser_results_t {
    ParseState parseState;
    char *full_msg;
    char *url;
} parser_res;

int parse_http_request(llhttp_t *parser, const char *data, int data_len);
void destroy_request_parser(llhttp_t *parser, parser_res *p_res);
int init_request_parser(llhttp_t *parser, parser_res *p_res);
int init_parser_res_structures(llhttp_t *parser, parser_res *con_req);
#endif // !__HTTP_PARSER__
