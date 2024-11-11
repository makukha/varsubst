import subprocess

import pytest


@pytest.mark.parametrize(
    'input,envvars,result', [
        ('${VAR}', {'VAR': 'value'}, b'value'),
        ('${VAR}', {}, b''),  # undefined variable substitutes empty string
    ]
)
def test(tmp_path, input, envvars, result):
    compose = tmp_path / 'compose.yaml'
    compose.write_text(f'''services:
    test:
      image: busybox
      command: echo -n '{input}'
    ''')
    out = subprocess.run(
        ['docker', 'compose', '-f', str(compose), 'run', '--rm', 'test'],
        env=envvars,
        capture_output=True,
    )
    assert out.returncode == 0 and out.stdout == result
