# Unit-testing static functions in C

This repo shows a method to write unit-tests for `static` functions in C.

C source files usually have a number of `static` functions. By definition,
these functions are local to the file and not available elsewhere. Many of
these functions are helper functions and ideal candidate for unit-tests -
except that you can't test them, they are local after all.

The most likely candidate that matches these criteria is the function that
parses your commandline arguments. Rarely unit-tested but complex enough to
need tests. But often declared as a `static void parse_args(...)` or
similar.

Options you have to test `static` functions in C code:

- move those functions into other files and/or declare them in headers, thus
  making them non-`static`. This is the best solution in many cases
  but you may end up polluting your internal namespace with all the new
  function names you now need to come up with.
- customize the build with e.g. `#ifdef TESTING` to change how these
  functions are accessible during test builds. I find this makes the code
  worse and harder to understand.
- `#define static /* * /` in your test build to make all static functions
  unconditionally public. You'll need `-Wno-implicit-function-declarations`
  to not get swamped by compiler warnings. This may not work if two `static`
  functions use the same name.
- ... ?

The solution illustrated here 
- can run against a single source file at a time,
- can easily setup different tests against the same source file,
- does not modify the source code at all,
- allows for easy mocking of functions

It relies on linker to ignore missing symbols (these are unused anyway) but
otherwise uses standard C features.

The solution outlined below is available in this repo, run `meson
builddir && ninja -C builddir &&  ninja -C builddir test` to reproduce.

## Example

Let's say our source file `example.c` contains the following
code:

```c
#include <stdio.h>
#include <stdbool.h>

static bool is_acceptable_id(unsigned int id) {

    if (database_id_exists(id))
        return 0;

    return id > 1000 && id < 10000;
}

void some_function(int argument) {
    function_somewhere_else(argument);
}

int main(int argc, char **argv) {
    int id = atoi(argv[1]);
    int is_acceptable = is_acceptable_id(id);

    printf("ID is acceptable: %d\n", is_acceptable);

    return 0;
}
```

`some_function()` can be unit-tested, but `is_acceptable_id()` is `static`.
But we really want to unit-test `is_acceptable_id()` that one, it's simple
and important. Writing tests for it means having to declare it properly,
possibly move it to a different file, etc. If you have worked on larger C
projects, you can probably think of a number of candidates analogous to this
example.

## Solution

The solution has three components: using `#include` to get access to the
static methods, moving `main` out of the way where necessary and a set of
linker flags to work around the resulting difficulties.

This approach does not restrict you in the type of test suite you can use.
In the examples below, I'll simply use `assert()`.

### Step 0: write the test

Let's write the test cases first, our `test-example.c` file looks like this:
```c
#include <assert.h>

#include <test-suite-header.h>
#include <other-stuff-you-need.h>

static void test_id(void) {
    assert(!is_acceptable_id(10));
    assert(!is_acceptable_id(100));
    assert(!is_acceptable_id(1000));

    assert(is_acceptable_id(5000));
    assert(is_acceptable_id(3000));
}

int main(int argc, char **argv)
{
    test_id();
    return 0;
}
```

Fairly straighforward. As mentioned above you are not restricted in the type
of test suite to use.

### Step 1: include the real source file

Including the source file gives us access to all the `static` functions
within the file:

```c
#include "example.c"    // <---- include the file

#include <assert.h>

#include <test-suite-header.h>
#include <other-stuff-you-need.h>

static void test_id(void) {
    assert(!is_acceptable_id(10));
    assert(!is_acceptable_id(100));
    assert(!is_acceptable_id(1000));

    assert(is_acceptable_id(5000));
    assert(is_acceptable_id(3000));
}

int main(int argc, char **argv)
{
    test_id();
    return 0;
}
```

Compile with `gcc -o test test-example.c` but of course this won't work
because we have two `main()` methods here. On to step 2 which is optional if
you are testing a source file without a `main()`.


### Step 2: move main out of the way

Skip this step if the source file doesn't have a `main()`.

Let's rename `main` to some other name with a `#define`. This one is best
supplied with a compiler argument (`-Dmain=_main_disabled`) so it applies to
all source files you provide. But we want our test's `main()` to work, so we
just `#undef main` before defining that one.

```c
#include "example.c"

#include <assert.h>

#include <test-suite-header.h>
#include <other-stuff-you-need.h>

static void test_id(void) {
    assert(!is_acceptable_id(10));
    assert(!is_acceptable_id(100));
    assert(!is_acceptable_id(1000));

    assert(is_acceptable_id(5000));
    assert(is_acceptable_id(3000));
}

#undef main                     // <--- stop renaming 'main'
int main(int argc, char **argv)
{
    test_id();
    return 0;
}
        
```

Compile this with `gcc -Dmain=_main_disabled -o test test-example.c` and
we're almost there, except we get a bunch of missing symbols. 

### Step 3: mock the missing pieces

We can't compile this file yet, because we're missing a bunch of functions
like `database_id_exists()`. Plus, this function is one that we need to mock
anyway because we don't have a database during testing.

So we mock that one:

```c
#include "example.c"

#include <test-suite-header.h>
#include <other-stuff-you-need.h>

bool database_id_exists(unsigned int id)        // <--- mock a missing function
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

#undef main
int main(int argc, char **argv)
{
    test_id();

    return 0;
}
```

Simple enough. Still won't work because we are missing all the 
functions defined in other source files, `function_somewhere_else()` in our
`example.c`.

### Step 4: hook up the compiler and linker

We still can't compile, because we don't have `function_somewhere_else()`.
But that one is never called by our test, so we just tell the linker to
ignore the error with `--unresolved-symbols=ignore-all`.

So our compiler command is now
`gcc -Dmain=_main_disabled -Wl,--unresolved-symbols=ignore-all -o test
test-example.c`.
And that's basically it, we now have `test-example.c` being a unit-test for
the static functions within `example.c`.

However, for (largely cosmetic) reasons you need a few more flags which are
illustrated in this [meson](https://mesonbuild.com) snippet:

```
e = executable('test-example',
               'example.c', 'test-example.c',  # the source files
               dependencies: [dep_ext_library],
               c_args: ['-Dmain=_main_disabled'
                        '-Wno-missing-prototypes'],
               link_args: ['-Wl,--unresolved-symbols=ignore-all',
                           '-Wl,-zmuldefs',
                           '-no-pie'],
               install: false),
)
test('test-example', e)
```

Full explanation of everything here:
- `executable` is meson lingo for "build an executable" and the first
  argument is the name of the resulting binary
- `example.c` and `test-example.c` are the input source files. You don't
  technically need `example.c` but it helps meson re-trigger the test built
  when that file changes.
- `dependencies` is the list of external/internal libraries we need to link
  to. You **must** include the dependencies whose header files you require,
  and any libraries you want to call into as part of your test.
- `c_args` are compiler flags. Both are only required if you are testing a
  source file with a `main()` method.
  - `-Dmain=_main_disabled` renames the `main()` to `_main_disabled()`
  - `-Wno-missing-prototypes` stops compiler warnings about missing
    prototypes (which you'd get for `_main_disabled()`).
- `link_args` are the linker flags:
    - `--unresolved-symbols=ignore-all`: tell the linker to ignore any
      unresolved symbols. This means even if `example.c` uses functions from
      some other source file, we can successfully link our test executable.
    - `-zmuldefs`: allow symbols to be defined more than once. `example.c`
      defines `multiply()` but it gets redefined when it is included in
      `test-example.c`. This triggers linker errors that we need to ignore.
      This is cosmetic only, you can drop `example.c` from the sources list
      instead.
    - `-no-pie`: required wherever `pie` is enabled by default (e.g. Arch).
      We'll likely end up with undefined symbols, position-independent code gets
      relocated on execution which will go boom if we have undefined symbols. So
      let's not do that for this test.

Alternatively, if you don't want to deal with unresolved symbols, just 
build with all the source files that the original binary needs. Mocking gets
harder but it may be sufficient for your use-cases. You'll only need
`-zmuldefs` then.

```
example = executable('example', <list of source files>)
e = executable('test-example',
               'test-example.c', <list of source files>,
               dependencies: [dep_ext_library],
               c_args: ['-Dmain=_main_disabled'
                        '-Wno-missing-prototypes'],
               link_args: ['-Wl,-zmuldefs'],
               install: false),
)
test('test-example', e)
```

Note how `test-example.c` is the first file. This way the mocked function is
defined first and the linker uses it instead of the real function defined in
the other file. 

*Note: I'm not sure if this is guaranteed linker behaviour and safe to rely
on*

## Caveats

It's a hack, so it's not as polished as it needs to be. Specifically:
- Linker errors are incredibly frustrating to debug. Since we turn off all
  the warnings, you won't get much feedback why things break. Best solution
  here is to try to add more dependencies until it works and removing the
  linker instructions to search in the warnings for something that isn't
  related to your code file.
- It's a re-build of the code so you're not testing the exact same bits as
  you ship. How much that is an issue depends on you and your use case.
