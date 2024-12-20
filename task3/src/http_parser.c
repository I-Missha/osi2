#include "http_parser.h"
#include "llhttp.h"
#include <stdint.h>
#include <string.h>

int on_url_complete(llhttp_t *parser) {
    parser_res *p_res = (parser_res *)parser->data;
    p_res->isParsed = PARSED;
    return 0;
}
void reallocate_mem(char **ptr, int new_size) {

    char *to_free = *ptr;
    *ptr = malloc(new_size * ALLOCATION_RATIO);
    strcpy(*ptr, to_free);
    free(to_free);
}

int on_url(llhttp_t *parser, const char *url_part, uint64_t addition_size) {
    parser_res *p_res = parser->data;
    char *url = p_res->url;
    int url_curr_size = p_res->url_curr_size;
    // todo: delete
    /*if (!url) {*/
    /*    p_res->url = (char *)malloc(sizeof(char) * URL_LIMIT);*/
    /*    url = p_res->url;*/
    /*}*/
    for (uint64_t i = 0; i < addition_size; i++) {
        url[i + url_curr_size] = url_part[i];
    }
    p_res->url_curr_size += addition_size;
    return 0;
}

int init_parser_res_structures(llhttp_t *parser, parser_res *con_req) {
    static llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_url = on_url;
    settings.on_url_complete = on_url_complete;
    con_req->url = (char *)malloc(sizeof(char) * URL_LIMIT);
    con_req->url_buff_size = URL_LIMIT;
    llhttp_init(parser, HTTP_BOTH, &settings);
    parser->data = con_req;
    return 0;
}

int parse_http_request(llhttp_t *parser, const char *data, int data_len) {
    if (!parser || !data) {
        return -1;
    }

    enum llhttp_errno err = llhttp_execute(parser, data, data_len);
    if (err != HPE_OK) {
        return 0;
    } else {
        return -1;
    }
}
