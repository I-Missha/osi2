#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__
#include <llhttp.h>
#include <stdio.h>
#include <stdlib.h>
int parse_http_request(const char *data, int data_len);
#endif // !__HTTP_PARSER__
