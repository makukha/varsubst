import os


project = 'vsub'
copyright = '2024, Michael Makukha'
author = 'Michael Makukha'
release = '0.2.0'

extensions = [
    'myst_parser',
]

templates_path = ['_templates']
exclude_patterns = []

html_theme = 'furo'
html_static_path = ['_static']

html_baseurl = os.environ.get('READTHEDOCS_CANONICAL_URL', '/')
html_js_files = [
    ('readthedocs.js', {'defer': 'defer'}),
]
