#include "http_parser.h"
#include "llhttp.h"
#include <stdint.h>
#include <string.h>

int on_url_complete(llhttp_t *parser) {
    connection_request *connection = (connection_request *)parser->data;
    connection->isParsed = PARSED;
    return 0;
}
void reallocate_mem(char **ptr, int new_size) {

    char *to_free = *ptr;
    *ptr = malloc(new_size * ALLOCATION_RATIO);
    strcpy(*ptr, to_free);
    free(to_free);
}

int on_url(llhttp_t *parser, const char *url_part, uint64_t addition_size) {
    connection_request *connection = parser->data;
    char *url = connection->url;
    int url_curr_size = connection->url_curr_size;
    // todo: delete
    /*if (!url) {*/
    /*    connection->url = (char *)malloc(sizeof(char) * URL_LIMIT);*/
    /*    url = connection->url;*/
    /*}*/
    if (connection->url_buff_size < addition_size + connection->url_buff_size) {
        reallocate_mem(
            (void **)&connection->url,
            (connection->url_curr_size + addition_size) * sizeof(char)
        );
        connection->url_buff_size +=
            (connection->url_curr_size + addition_size) * ALLOCATION_RATIO;
    }
    for (uint64_t i = 0; i < addition_size; i++) {
        url[i + url_curr_size] = url_part[i];
    }
    connection->url_curr_size += addition_size;
    return 0;
}

int init_connection_request_structures(
    llhttp_t *parser, connection_request *con_req
) {
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
