#ifndef POST_PARSE_H
#define POST_PARSE_H
#include "sparse_matrix.h"

char* parse_post_request(const char *str_request, const char *str_regex);
coo_entry_t* get_rating_from_http(const char *str_request, const char *str_regex);
#endif
