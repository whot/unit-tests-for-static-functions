int main(int, char **);
#include "example.c"

#include <assert.h>

/* Include other headers you need as you'd expect it*/


bool database_id_exists(unsigned int id)
{
    return false;
}

static void test_id(void) {
    assert(!is_acceptable_id(10));
    assert(!is_acceptable_id(100));
    assert(!is_acceptable_id(1000));

    assert(is_acceptable_id(5000));
    assert(is_acceptable_id(3000));
}

/* Remove the define for main=_main_disabled so we can actually supply our
   own main method. */
#undef main
int main(int argc, char **argv)
{
    test_id();

    return 0;
}
