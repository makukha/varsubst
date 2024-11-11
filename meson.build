project(
    'vsub', 'c',
    version: '0.1.0',
    license: 'MIT',
    license_files: ['LICENSE'],
    default_options: ['c_std=c11'],
)
src = files(
    'src/detail.c',
    'src/main.c',
    'src/util.c',
    'src/vsub.c',
    'src/vsubio.c',
    'src/input/text_file.c',
    'src/input/text_str.c',
    'src/input/vars_arrays.c',
    'src/input/vars_env.c',
    'src/input/vars_kvarray.c',
    'src/output/json.c',
    'src/output/plain.c',
    'src/output/pretty.c',
    'src/syntax/default/parser.c',
)
deps = [
    dependency('libcjson', version: '>=1.7.18', static: true),
]

library('vsub', src, dependencies: deps, install: true)
executable('vsub', src, dependencies: deps, install: true)
install_headers('src/vsub.h')
