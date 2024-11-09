from dataclasses import dataclass
import json
from itertools import chain
from pathlib import Path
import subprocess
from subprocess import getoutput

import pytest


@dataclass
class BuildInfo:
    dir: Path
    src: Path
    mesonfile: Path
    projectinfo: dict[str]
    targets: list[dict[str]]


@pytest.fixture(scope='session')
def build() -> BuildInfo:
    builddir = Path('build-release').resolve()
    return BuildInfo(
        dir=builddir,
        src=Path('src').resolve(),
        mesonfile=Path('meson.build').resolve(),
        projectinfo=json.loads(getoutput(f'meson introspect --projectinfo {builddir}')),
        targets=json.loads(getoutput(f'meson introspect --targets {builddir}')),
    )


@dataclass
class Executable:
    name: str
    home: Path
    version: str
    formats: list[str]
    syntaxes: list[str]

    @property
    def path(self) -> Path:
        return self.home / self.name

    def __post_init__(self):
        assert (self.home / self.name).is_file()

    def __str__(self):
        return str(self.path)

    def getoutput(self, cmdline: str, encoding: str | None = 'utf-8') -> str | bytes:
        res = subprocess.run(
            ['sh', '-c', cmdline],
            capture_output=True,
            encoding=encoding,
        )
        assert res.returncode == 0
        return res.stdout


@pytest.fixture(scope='session')
def exe(build) -> Executable:
    return Executable(
        name=[
            t['name'] for t in build.targets
            if t['type'] == 'executable' and t['defined_in'] == str(build.mesonfile)
        ][0],
        home=build.dir,
        version=build.projectinfo['version'],
        formats=[p.stem for p in (build.src / 'output').glob('*.c')],
        syntaxes=[p.name for p in (build.src / 'syntax').iterdir() if p.is_dir()],
    )
