from collections.abc import Sequence
from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable, Iterable, assert_never

import pytest
from _pytest.mark import ParameterSet


class VarAction(Enum):
    USE_ORIGINAL = auto()
    USE_EMPTY = auto()
    ERROR = auto()


class VarPattern(Enum):
    ASCII_IDENTIFIER = auto()
    UNICODE_IDENTIFIER = auto()


@dataclass(kw_only=True)
class Syntax:
    named_form: bool               # supports named form $VAR
    braced_form: bool              # supports braced form ${VAR}
    dollar_escape: str | None      # escape character or False if not supported
    dollar_literal: VarAction      # semantics of standalone dollar character
    recursive_depth: int | float   # default depth of iterative substitution or float(Inf)
    var_pattern: VarPattern        # var name pattern
    var_invalid: VarAction         # behaviour if template contains invalid var name
    var_unset: VarAction           # behaviour if template contains unset var
    var_case_sensitive: bool       # var name is case-insensitive
    skip: Sequence[Callable[[ParameterSet], bool]] | None = None
    more: Sequence[Callable[[], Iterable[ParameterSet]]] | None = None

    def __post_init__(self):
        for feature, invalid_values in (
            ('dollar_literal', (VarAction.USE_EMPTY,)),
            ('var_invalid', ()),
            ('var_unset', ()),
        ):
            value = getattr(self, feature)
            if value in invalid_values:
                raise ValueError(f'{feature} value is invalid: {value}')

    def pytest_params(self) -> ParameterSet:
        # filter marked as skipped
        if self.skip is None:
            yield from self.cases()
        else:
            for param in self.cases():
                if any(must_skip(param) for must_skip in self.skip):
                    param.marks = (pytest.mark.skip(),)
                yield param
        # add extras
        if self.more:
            for gen in self.more:
                yield from gen()

    def cases(self) -> Iterable[ParameterSet]:

        # var_pattern, var_invalid
        var_valid, var_invalid = self.var_names()

        # named_form, braced_form
        for form, expr in (
            (self.named_form, '${0}'),
            (self.braced_form, '${{{0}}}'),
        ):
            for name in (*var_valid, *var_invalid):
                case = Case(expr.format(name), {name: 'value'})
                if form and name in var_valid:
                    yield case.expect_value('value')
                elif form and name in var_invalid:
                    yield case.expect_action(self.var_invalid)
                else:
                    yield case.expect_action(self.dollar_literal)

        # dollar_escape
        escapes = {*(self.dollar_escape or ()), '$', '\\'}
        for form, expr in (
            (self.named_form, '${0}'),
            (self.braced_form, '${{{0}}}'),
        ):
            for name in (*var_valid, *var_invalid):
                for esc in escapes:
                    case = Case(f'{esc}{expr.format(name)}', {name: 'value'})
                    if esc == self.dollar_escape:
                        yield case.expect_input()
                    elif esc != self.dollar_escape and name in var_valid:
                        yield case.expect_value(f'{esc}value')
                    elif esc != self.dollar_escape and name in var_invalid:
                        match self.var_invalid:
                            case VarAction.USE_ORIGINAL:
                                yield case.expect_input()
                            case VarAction.USE_EMPTY:
                                yield case.expect_value(esc)
                            case VarAction.ERROR:
                                yield case.expect_action(self.var_invalid)
                            case _ as unreachable:
                                assert_never(unreachable)

        # dollar_literal (middle, last), w/wo escape


        # recursive depth

        # var_unset

        # var_case_sensitive
        for form, expr in (
            (self.named_form, '${0}'),
            (self.braced_form, '${{{0}}}'),
        ):
            if not form:
                continue  # unsupported forms were tested above
            for name in var_valid:
                for case in (
                    Case(f'${name.upper()}', {name.lower(): 'value'}),
                    Case(f'${name.lower()}', {name.upper(): 'value'}),
                ):
                    if self.var_case_sensitive:
                        yield case.expect_action(self.var_unset)
                    else:
                        yield case.expect_value('value')

    def var_names(self) -> tuple[tuple[str, ...], tuple[str, ...]]:
        ascii_ident = ('VAR_1', '_VAR', '_1VAR')
        unicode_ident = ('ПРМ', '木')
        digit = ('1VAR', '1')
        special = ('-', '@', '}')  # "{" is special, add to tests to get more info
        match self.var_pattern:
            case VarPattern.ASCII_IDENTIFIER:
                return (ascii_ident, (*unicode_ident, *digit, *special))
            case VarPattern.UNICODE_IDENTIFIER:
                return ((*ascii_ident, *unicode_ident), (*digit, *special))
            case _:
                assert_never(self.var_pattern)


@dataclass
class Case:
    input: str
    vars: dict[str, str]

    def expect_action(self, action: VarAction) -> pytest.param:
        match action:
            case VarAction.USE_ORIGINAL:
                return self.expect_input()
            case VarAction.USE_EMPTY:
                return self.expect_value('')
            case VarAction.ERROR:
                return self.expect_error(None)
            case _:
                assert_never(action)

    def expect_input(self):
        return pytest.param(self.input, self.vars, self.input)

    def expect_value(self, value: str):
        return pytest.param(self.input, self.vars, value)

    def expect_error(self, msg: str | None) -> pytest.param:
        xfail = pytest.mark.xfail(msg) if msg else pytest.mark.xfail
        return pytest.param(self.input, self.vars, None, marks=xfail)
