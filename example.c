#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "example.h"
#include "otherfile.h"

static bool is_acceptable_id(unsigned int id) {

    if (database_id_exists(id))
        return 0;

    return id > 1000 && id < 10000;
}

void some_function(int argument) {
    printf("..... %s wasn't supposed to be called\n", __func__);
    function_somewhere_else(argument);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <id>\n", basename(argv[0]));
        return 1;
    }
    int id = atoi(argv[1]);
    int is_acceptable = is_acceptable_id(id);

    printf("ID is acceptable: %d\n", is_acceptable);

    return 0;
}
