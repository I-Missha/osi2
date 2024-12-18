#include "http_parser.h"
#include "llhttp.h"

int parse_http_request(const char *data, int data_len) {
    llhttp_settings_t settings;
    llhttp_t parser;
    llhttp_settings_init(&settings);
    llhttp_init(&parser, HTTP_BOTH, &settings);
    enum llhttp_errno err = llhttp_execute(&parser, data, data_len);
    printf("here\n");
    if (err != HPE_OK) {
        for (int i = 0; i < data_len; i++) {
            printf("%c", *((char *)parser.data + i));
        }
        printf("\n");
        return 0;
    } else {
        return -1;
    }
}
