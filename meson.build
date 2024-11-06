project(
    'vsub', 'c',
    version: '0.1.0',
    license: 'MIT',
    license_files: ['LICENSE'],
)
src = files(
    'src/main.c',
    'src/vsub.c',
    'src/input/text_file.c',
    'src/input/text_str.c',
    'src/input/vars_arglist.c',
    'src/input/vars_arrays.c',
    'src/input/vars_env.c',
    'src/syntax/default/parser.c',
)

library('vsub', src, install: true)
executable('vsub', src, install: true)
install_headers('src/vsub.h')
