#include "http_parser.h"
#include "llhttp.h"
#include <stdint.h>
#define URL_LIMIT 2048
#define PARSED 1
typedef struct connection_request {
    uint8_t isParsed;
    char *url;
    int url_curr_size;
} connection_request;

int on_url_complete(llhttp_t *parser) {
    connection_request *connection = (connection_request *)parser->data;
    connection->isParsed = PARSED;
    return 0;
}

int on_url(llhttp_t *parser, const char *url_part, uint64_t length) {
    connection_request *connection = parser->data;
    char *url = connection->url;
    int url_curr_size = connection->url_curr_size;
    if (!url) {
        connection->url = (char *)malloc(sizeof(char) * URL_LIMIT);
        url = connection->url;
    }
    for (uint64_t i = 0; i < length; i++) {
        url[i + url_curr_size] = url_part[i];
    }
    printf("here %llu \n", length);
    connection->url_curr_size += length;
    return 0;
}
int parse_http_request(const char *data, int data_len) {
    // ToDo: make a http_init
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_url = on_url;
    settings.on_url_complete = on_url_complete;
    llhttp_t parser;
    connection_request req = {0, NULL, 0};
    llhttp_init(&parser, HTTP_BOTH, &settings);
    parser.data = &req;
    enum llhttp_errno err = llhttp_execute(&parser, data, data_len);
    if (err != HPE_OK) {
        return 0;
    } else {
        return -1;
    }
}
