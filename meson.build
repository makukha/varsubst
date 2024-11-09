project(
    'vsub', 'c',
    version: '0.1.0',
    license: 'MIT',
    license_files: ['LICENSE'],
)
src = files(
    'src/detail.c',
    'src/error.c',
    'src/main.c',
    'src/output/json.c',
    'src/output/plain.c',
    'src/util.c',
    'src/vsub.c',
    'src/input/text_file.c',
    'src/input/text_str.c',
    'src/input/vars_arglist.c',
    'src/input/vars_arrays.c',
    'src/input/vars_env.c',
    'src/syntax/default/parser.c',
)
deps = [
    dependency('libcjson', version: '>=1.7.18'),
]

library('vsub', src, dependencies: deps, install: true)
executable('vsub', src, dependencies: deps, install: true)
install_headers('src/vsub.h')
