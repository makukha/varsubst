from pathlib import Path
import subprocess

import pytest

from syntax import Syntax, VarPattern, VarEmptyAct, VarUnsetAct, VarInvalidAct


SYNTAX = Syntax(
    named_form=True,
    braced_form=True,
    dollar_escape=False,
    recursive_depth=0,
    var_pattern=VarPattern.ASCII_IDENTIFIER,
    var_invalid=VarInvalidAct.USE_ORIGINAL,  # todo:
    var_unset=VarUnsetAct.USE_EMPTY,
    var_empty=VarEmptyAct.USE_SUBSTITUTE,
    var_name_ignorecase=False,
)


@pytest.mark.parametrize('input,vars,result', list(SYNTAX.pytest_params()))
def test(tmp_path: Path, input, vars, result):

    infile = tmp_path / 'input.txt'
    infile.write_text(input, encoding='utf-8')

    envfile = tmp_path / '.env'
    envfile.write_text(
        '\n'.join(f'export {k}="{v}"' for k, v in vars.items()),
        encoding='utf-8',
    )

    # check var name to be valid in shell
    out = subprocess.run(['sh', '-c', f'. {envfile}'])
    if out.returncode != 0:
        pytest.xfail()  # invalid environment variable, test will fail

    # test allowed var names
    out = subprocess.run(
        [
            'env', '-i', 'sh', '-c',
            f'unset PWD && . {envfile} && /bin/cat {infile} | /usr/bin/envsubst',
        ],
        capture_output=True,
        text=True,
    )

    if result:
        assert out.returncode == 0
        assert out.stdout == result
    else:
        assert out.returncode != 0
