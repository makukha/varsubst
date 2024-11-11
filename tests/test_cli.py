from pathlib import Path

import pytest

from conftest import Executable


# helpers

def assert_help(exe, out):
    assert out.returncode == 0
    assert out.stdout.startswith('usage: ')


def assert_version(exe, out):
    assert out.returncode == 0
    assert out.stdout == f'{exe.name} {exe.version}\n'


def assert_formats(exe, out):
    assert out.returncode == 0
    assert set(out.stdout.split()) == set(exe.formats)


def assert_syntaxes(exe, out):
    assert out.returncode == 0
    syntaxes = [ln.split()[0] for ln in out.stdout.splitlines()]
    assert set(syntaxes) == set(exe.syntaxes)


def assert_invalid_option(exe, out):
    assert out.returncode != 0
    assert out.stderr.startswith('invalid option: ')


# simple options

def test_help(exe: Executable):
    for opt in ['-h', '--help']:
        out = exe.run(f'{exe} {opt}')
        assert_help(exe, out)


def test_version(exe: Executable):
    out = exe.run(f'{exe} --version')
    assert_version(exe, out)


def test_formats(exe: Executable):
    out = exe.run(f'{exe} --formats')
    assert_formats(exe, out)


def test_syntaxes(exe: Executable):
    out = exe.run(f'{exe} --syntaxes')
    assert_syntaxes(exe, out)


@pytest.mark.parametrize(
    'args,assertion', [
        # help
        ('--help --whatever', assert_help),
        ('--dummy --help', assert_invalid_option),
        ('-hZ', assert_help),
        ('-Zh', assert_invalid_option),
        # version
        ('--version --whatever', assert_version),
        ('--dummy --version', assert_invalid_option),
        # formats
        ('--formats --whatever', assert_formats),
        ('--dummy --formats', assert_invalid_option),
        # syntaxes
        ('--syntaxes --whatever', assert_syntaxes),
        ('--dummy --syntaxes', assert_invalid_option),
    ]
)
def test_options_precedence(exe: Executable, args, assertion):
    for paths in ('', 'p1', 'p1 p2'):
        out = exe.run(f'{exe} {args} {paths}')
        assertion(exe, out)


# argument errors

@pytest.mark.parametrize(
    'args,output', [
        # multiple paths
        ('path1 path2', b'multiple paths not allowed\n'),
        ('p1 p2 p3', b'multiple paths not allowed\n'),
        # invalid option
        ('--dummy', b'invalid option: --dummy\n'),
        ('--dummy path', b'invalid option: --dummy\n'),
        ('-Z', b'invalid option: -Z\n'),
        ('-eZ', b'invalid option: -eZ\n'),  # '-e' is valid option
        ('-Z path', b'invalid option: -Z\n'),
        ('-Z path1 path2', b'invalid option: -Z\n'),
        ('--syntax', b'invalid option: --syntax\n'),  # missing argument of valid option
        # unsupported syntax
        ('-s dummy', b'unsupported syntax: dummy\n'),
        ('-sdummy', b'unsupported syntax: dummy\n'),
        ('--syntax dummy', b'unsupported syntax: dummy\n'),
        ('--syntax=dummy', b'unsupported syntax: dummy\n'),
        # unable to open file: see test_file_missing()
        # unsupported output format
        ('--format dummy', b'unsupported output format: dummy\n'),
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
