#include <stdlib.h>
#include <stdio.h>
#include "otherfile.h"

void function_somewhere_else(int argument) {
    /* does not get called, this illustrates a function elsewher in your
     * project. We abort here to illustrate that this one really never gets
     * called */
    printf("..... %s wasn't supposed to be called\n", __func__);
    abort();
}

bool database_id_exists(unsigned int id) {
    /* let's pretend this is the db connection code and whatnot */

    /* we abort here to illustrate that it's really the mocked function that
     * gets called, not this one */
    printf("..... %s wasn't supposed to be called\n", __func__);
    abort();
    return true;
}
