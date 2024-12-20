#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__
#include <llhttp.h>
#include <stdio.h>
#include <stdlib.h>
#define URL_LIMIT 2048
#define ALLOCATION_RATIO 2
#define PARSED 1
typedef struct parser_results_t {
    uint8_t isParsed;
    char *url;
    int url_curr_size;
    int url_buff_size;
} parser_res;
int parse_http_request(llhttp_t *parser, const char *data, int data_len);
int init_parser_res_structures(llhttp_t *parser, parser_res *con_req);
#endif // !__HTTP_PARSER__
