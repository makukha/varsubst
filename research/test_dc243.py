import json
import re
import subprocess

import pytest

from syntax import Case, Syntax, VarAction, VarPattern


def skip_var_named_invalid(param):
    """Docker Compose parser treats unescaped dollar char followed by
    NAMED invalid variable as incorrectly spelled literal and quotes it: "$}" => "$$}".
    At the same time, dollar char followed by BRACED invalid variable name
    is treated as an error.
    """
    if re.search(r'(?<![$])[$][^[_a-zA-Z]]', param.values[0]):
        return True


def case_var_named_invalid():
    _, var_invalid = SYNTAX.var_names()
    for name in var_invalid:
        yield Case(f'${name}', {}).expect_value(f'$${name}')


SYNTAX = Syntax(
    named_form=True,
    braced_form=True,
    dollar_escape='$',
    dollar_literal=VarAction.USE_ORIGINAL,    # todo: proof required
    recursive_depth=float('Inf'),             # todo: proof required
    var_pattern=VarPattern.ASCII_IDENTIFIER,
    var_invalid=VarAction.ERROR,
    var_unset=VarAction.USE_EMPTY,
    var_case_sensitive=True,
    skip=(
        skip_var_named_invalid,
    ),
    more=(
        case_var_named_invalid,
    ),
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
