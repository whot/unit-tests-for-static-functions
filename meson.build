project('static-tests', 'c',
	default_options : [ 'warning_level=2' ],
        license: 'Public Domain')

# Our "real" binary. Just here of illustrative purposes.
executable('example',
           'example.c', 'otherfile.c',
           install: false)

# The test binary, only uses one of the source files of the real binary
t = executable('test-example',
               'example.c', 'test-example.c',
               c_args: ['-Dmain=_main_disabled',
                        '-Wno-missing-prototypes'],
               link_args: ['-Wl,--unresolved-symbols=ignore-all',
                           '-Wl,-zmuldefs',
                           '-no-pie'],
               install: false)
test('test-example', t)

# Test binary using all source files so we don't need to work around missing
# symbols.
t = executable('test-example-full',
               'test-example.c', 'example.c', 'otherfile.c',
               c_args: ['-Dmain=_main_disabled',
                        '-Wno-missing-prototypes'],
               link_args: ['-Wl,-zmuldefs'],
               install: false)
test('test-example-full', t)
