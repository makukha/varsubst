from dataclasses import dataclass
from enum import StrEnum, auto

import pytest


@dataclass
class Cases:
    valid: tuple[str, ...]
    invalid: tuple[str, ...]


class VarPattern(StrEnum):
    ASCII_IDENTIFIER = auto()
    UNICODE_IDENTIFIER = auto()


class VarUnsetAct(StrEnum):
    USE_ORIGINAL = auto()
    USE_EMPTY = auto()
    ERROR = auto()


class VarEmptyAct(StrEnum):
    USE_ORIGINAL = auto()
    USE_SUBSTITUTE = auto()
    ERROR = auto()


class VarInvalidAct(StrEnum):
    USE_ORIGINAL = auto()
    ERROR = auto()


@dataclass
class Syntax:
    named_form: bool              # supports named form $VAR
    braced_form: bool             # supports braced form ${VAR}
    dollar_escape: str | bool     # escape character or False if not supported
    recursive_depth: int | float  # default depth of iterative substitution or float(Inf)
    var_pattern: VarPattern       # var name pattern
    var_invalid: VarInvalidAct    # behaviour if template contains invalid var name
    var_unset: VarUnsetAct        # behaviour if template contains unset var
    var_empty: VarEmptyAct        # behaviour if template contains empty var
    var_name_ignorecase: bool     # whether var name is case-insensitive

    def pytest_params(self) -> list[tuple[str, dict[str, str], str | None]]:
        varnames = self.var_name_cases()

        for supported_form, input in (
            (self.named_form, '${0}'),
            (self.braced_form, '${{{0}}}'),
        ):
            if supported_form:
                for name in varnames.valid:
                    inp = input.format(name)
                    yield pytest.param(inp, {name: 'value'}, 'value')

                for name in varnames.invalid:
                    inp = input.format(name)
                    if self.var_invalid == VarInvalidAct.ERROR:
                        res, marks = None, {'marks': pytest.mark.xfail}
                    else:
                        res, marks = inp, {}
                    yield pytest.param(inp, {name: 'value'}, res, **marks)

            else:
                for name in varnames.valid:
                    inp = input.format(name)
                    yield pytest.param(inp, {name: 'value'}, None)

    def var_name_cases(self) -> Cases:
        # note: don't include '"' character in var name, it may break tests
        unicode_names = ('ПРМ', '木')
        special_names = ('-', '@')
        digit_names = ('1VAR', '1')

        ascii_identifier = Cases(
            valid=('VAR_1', '_VAR', '_1VAR'),
            invalid=(*digit_names, *special_names, *unicode_names),
        )
        unicode_identifier = Cases(
            valid=('ПРМ', *ascii_identifier.valid),
            invalid=(*digit_names, *special_names),
        )

        if self.var_pattern == VarPattern.ASCII_IDENTIFIER:
            return ascii_identifier
        elif self.var_pattern == VarPattern.UNICODE_IDENTIFIER:
            return unicode_identifier
