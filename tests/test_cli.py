from pathlib import Path

import pytest

from conftest import Executable


# simple options

def test_help(exe: Executable):
    for opt in ['-h', '--help']:
        out = exe.run(f'{exe} {opt}')
        assert out.returncode == 0
        assert out.stdout.startswith('usage: ')


def test_version(exe: Executable):
    out = exe.run(f'{exe} --version')
    assert out.returncode == 0
    assert out.stdout == f'{exe.name} {exe.version}\n'


def test_formats(exe: Executable):
    out = exe.run(f'{exe} --formats')
    assert out.returncode == 0
    assert set(out.stdout.split()) == set(exe.formats)


def test_syntaxes(exe: Executable):
    out = exe.run(f'{exe} --syntaxes')
    assert out.returncode == 0
    syntaxes = [ln.split()[0] for ln in out.stdout.splitlines()]
    assert set(syntaxes) == set(exe.syntaxes)


# argument errors

@pytest.mark.parametrize(
    'args,output', [
        # multiple paths
        ('path1 path2', b'multiple paths not allowed'),
        ('p1 p2 p3', b'multiple paths not allowed'),
        # invalid option
        ('--dummy', b'invalid option: --dummy'),
        ('--dummy path', b'invalid option: --dummy'),
        ('-Z', b'invalid option: -Z'),
        ('-eZ', b'invalid option: -eZ'),  # '-e' is a valid option
        ('-Z path', b'invalid option: -Z'),
        ('-Z path1 path2', b'invalid option: -Z'),
        ('--syntax', b'invalid option: --syntax'),  # missing argument of valid option
        # unsupported syntax
        ('-s dummy', b'unsupported syntax: dummy'),
        ('-sdummy', b'unsupported syntax: dummy'),
        ('--syntax dummy', b'unsupported syntax: dummy'),
        ('--syntax=dummy', b'unsupported syntax: dummy'),
        # unable to open file: see test_file_missing()
        # unsupported output format
        ('--format dummy', b'unsupported output format: dummy'),
        # todo: errors generated by output
    ])
def test_multiple_paths(exe: Executable, args: str, output: bytes):
    out = exe.run(f'{exe} {args}', encoding=None)
    assert out.returncode != 0
    assert out.stderr == output


# file errors

def test_file_missing(exe: Executable, tmp_path: Path):
    fn = tmp_path / 'missing.txt'
    out = exe.run(f'{exe} {fn}')
    assert out.returncode != 0
    assert out.stderr == f'unable to open file: {fn}\n'
