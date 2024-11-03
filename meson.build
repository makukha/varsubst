project(
    'vsub', 'c',
    version: '0.1.0',
    license: 'MIT',
    license_files: ['LICENSE'],
)
src = files(
    'src/const.c',
    'src/main.c',
    'src/vsub.c',
    'src/syntax/default/parser.c',
)

library('vsub', src, install: true)
executable('vsub', src, install: true)
install_headers('src/vsub.h')
