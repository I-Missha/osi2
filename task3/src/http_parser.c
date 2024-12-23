#include "http_parser.h"
#include "vec.h"

int on_request_complete(llhttp_t *parser) {
    Parser_res *p_res = (Parser_res *)parser->data;
    const ParseState state = Parsed;
    p_res->parseState = state;
    return 0;
}

int on_method_complete(llhttp_t *parser) {
    Parser_res *parser_res = parser->data;
    parser_res->method = llhttp_get_method(parser);
    return 0;
}

int on_version_complete(llhttp_t *parser) {
    Parser_res *parser_res = parser->data;
    parser_res->major_version = llhttp_get_http_major(parser);
    parser_res->minor_version = llhttp_get_http_minor(parser);
    return 0;
}

int on_url(llhttp_t *parser, const char *url_part, uint64_t addition_size) {
    Parser_res *parser_res = parser->data;
    for (uint64_t i = 0; i < addition_size; i++) {
        vector_add(&parser_res->url, url_part[i]);
    }
    return 0;
}

int init_request_parser(
    llhttp_t *parser, Parser_res *p_res, llhttp_type_t http_type
) {
    static llhttp_settings_t settings;

    llhttp_settings_init(&settings);
    settings.on_url = on_url;
    settings.on_message_complete = on_request_complete;
    settings.on_method_complete = on_method_complete;
    settings.on_version_complete = on_version_complete;

    p_res->full_msg = vector_create();
    p_res->url = vector_create();
    const ParseState state = NotParsed;
    p_res->parseState = state;

    llhttp_init(parser, http_type, &settings);
    parser->data = p_res;
    return 0;
}

void destroy_request_parser(llhttp_t *parser, Parser_res *p_res) {
    vector_free(p_res->full_msg);
    llhttp_finish(parser);
    vector_free(p_res->url);
}

void http_parse_host_name(const char *url, char **host_name) {
    const char *start = strstr(url, "http://");
    if (start) {
        start += strlen("http://");
    } else {
        start = url;
    }
    const char *end = strchr(start, '/'); // Find the first '/' after the host

    for (const char *ptr = start; ptr != end && *ptr != '\0'; ++ptr) {
        vector_add(host_name, *ptr);
    }

    vector_add(host_name, '\0');
    printf("%s\n", *host_name);
}

int parse_http_request(llhttp_t *parser, const char *data, int data_len) {
    if (!parser || !data) {
        return 1;
    }

    enum llhttp_errno err = llhttp_execute(parser, data, data_len);
    if (err != HPE_OK) {
        return 1;
    }
    return 0;
}

void vector_push_str(char **vec, char *str, int str_size) {
    for (int i = 0; i < str_size; i++) {
        vector_add(vec, str[i]);
    }
}

int receive_parsed_request(int client_fd, llhttp_t *parser, Parser_res *p_res) {
    const ParseState state = Parsed;
    char *buff = malloc(BUFFER_SIZE);
    while (p_res->parseState != state) {
        int rec_size = read(client_fd, buff, BUFFER_SIZE);
        vector_push_str(&p_res->full_msg, buff, rec_size);
        if (parse_http_request(parser, buff, rec_size)) {
            return PARSE_ERROR;
        }
    }
    return SUCCESS_PARSE;
}
