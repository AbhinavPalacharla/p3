#include "utils.h"
#include <string.h>

char *strip(char *str) {
    if(str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0';
    }

    return str;
}