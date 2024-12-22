#include "http_parser.h"
#include "llhttp.h"
#include "vec.h"

/*int on_url_complete(llhttp_t *parser) {*/
/*    parser_res *p_res = (parser_res *)parser->data;*/
/*    p_res->isParsed = PARSED;*/
/*    return 0;*/
/*}*/

int on_request_complete(llhttp_t *parser) {
    parser_res *p_res = (parser_res *)parser->data;
    const ParseState state = ReqParsed;
    p_res->parseState = state;
    return 0;
}

int on_url(llhttp_t *parser, const char *url_part, uint64_t addition_size) {
    parser_res *parser_res = parser->data;
    for (uint64_t i = 0; i < addition_size; i++) {
        vector_add(&parser_res->url, url_part[i]);
    }
    return 0;
}

int init_request_parser(llhttp_t *parser, parser_res *p_res) {
    static llhttp_settings_t settings;

    llhttp_settings_init(&settings);
    settings.on_url = on_url;
    settings.on_message_complete = on_request_complete;

    p_res->url = vector_create();
    p_res->full_msg = vector_create();
    const ParseState state = NotParsed;
    p_res->parseState = state;

    llhttp_init(parser, HTTP_REQUEST, &settings);
    parser->data = p_res;
    return 0;
}

void destroy_request_parser(llhttp_t *parser, parser_res *p_res) {
    llhttp_finish(parser);
    vector_free(p_res->full_msg);
    vector_free(p_res->url);
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
