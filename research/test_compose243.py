import json
import subprocess

import pytest

from syntax import Syntax, VarAction, VarPattern


SYNTAX = Syntax(
    named_form=True,
    braced_form=True,
    unclosed_brace=VarAction.ERROR,
    dollar_escape='$',
    dollar_literal=VarAction.USE_ESCAPED,
    var_pattern=VarPattern.ASCII_IDENTIFIER,
    var_case_sensitive=True,
    var_invalid=VarAction.ERROR,
    var_unset=VarAction.USE_EMPTY,
)


@pytest.mark.parametrize('input,vars,result', list(SYNTAX.pytest_params()))
def test(input, vars, result):

    out = subprocess.run(
        ['/usr/bin/docker', 'compose', '-f-', 'config', '--format=json'],
        input=f'x-test: {json.dumps(input)}',
        env=vars,
        capture_output=True,
        text=True,
    )
    if result is None:
        assert out.returncode != 0
    else:
        assert out.returncode == 0
        assert json.loads(out.stdout)['x-test'] == result
