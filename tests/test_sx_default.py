import pytest


@pytest.mark.parametrize(
    'input,result,vars', [
        ('plaintext', b'plaintext', ''),
        ('${VAR}iable', b'viable', '-v VAR=v'),
        ('$VAR-iable', b'v-iable', '-v VAR=v'),
        ('$$VAR-iable', b'$VAR-iable', '-v VAR=v'),
        ('${UNDEF}-ined', b'${UNDEF}-ined', ''),
        ('${}plain', b'${}plain', ''),
        ('${-}plain', b'${-}plain', ''),
    ]
)
def test_simple(exe, input, vars, result):
    out = exe.run(f'echo -n \'{input}\' | {exe} {vars}', encoding=None)
    assert out.returncode == 0
    assert out.stdout == result
