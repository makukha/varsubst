import pytest


@pytest.mark.parametrize(
    'input,result,vars', [
        ('plaintext', b'plaintext', ''),
        ('${VAR}iable', b'viable', '-v VAR=v'),
        # todo: add more
    ]
)
def test_simple(exe, input, vars, result):
    out = exe.run(f'echo -n \'{input}\' | {exe} {vars}', encoding=None)
    assert out.returncode == 0
    assert out.stdout == result
