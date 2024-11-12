import subprocess

import pytest

from syntax import Syntax, VarAction, VarPattern


SYNTAX = Syntax(
    named_form=True,
    braced_form=True,
    dollar_escape=None,
    dollar_literal=VarAction.USE_ORIGINAL,
    recursive_depth=0,
    var_pattern=VarPattern.ASCII_IDENTIFIER,
    var_invalid=VarAction.USE_ORIGINAL,
    var_unset=VarAction.USE_EMPTY,
    var_case_sensitive=True,
)


@pytest.mark.parametrize('input,vars,result', list(SYNTAX.pytest_params()))
def test(input, vars, result):
    out = subprocess.run(
        ['/usr/bin/envsubst'],
        input=input,
        env=vars,
        capture_output=True,
        text=True,
    )
    if result is None:
        assert out.returncode != 0
    else:
        assert out.returncode == 0
        assert out.stdout == result
