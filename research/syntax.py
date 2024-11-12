from collections.abc import Sequence
from dataclasses import dataclass
from enum import Enum, auto
from itertools import product
from typing import Callable, ClassVar, Iterable, Self, assert_never

import pytest
from _pytest.mark import ParameterSet


class VarAction(Enum):
    USE_ORIGINAL = auto()
    USE_ESCAPED = auto()
    USE_EMPTY = auto()
    ERROR = auto()


class VarPattern(Enum):
    ASCII_IDENTIFIER = auto()
    UNICODE_IDENTIFIER = auto()


@dataclass(kw_only=True)
class Templates:
    # var names
    var_valid: list[str]    # valid var names
    var_invalid: list[str]  # invalid var names
    var_all: list[str]      # all var names to check
    # dollar escape
    esc_no: list[str]       # well-known dollar escape chars that are non-escapes here
    esc_maybe: list[str]    # all characters to try as dollar escapes
    # dollar atomic expressions
    expr_named: ClassVar[str] = '${0}'
    expr_braced: ClassVar[str] = '${{{0}}}'
    expr_var: list[str]     # dollar str.format templates of valid variable expressions
    expr_lit: list[str]     # dollar str.format templates that are not var expressions
    expr_all: list[str]     # expr_var + exp_lit

    @classmethod
    def from_syntax(cls, synt: 'Syntax') -> Self:
        # var names
        ascii_ident = ['VAR_1', '_VAR', '_1VAR']
        unicode_ident = ['ПРМ', '木']
        digit = ['1VAR', '1']
        special = ['-', '@', '}']  # "{" is special, add to tests to get more info from errors
        match synt.var_pattern:
            case VarPattern.ASCII_IDENTIFIER:
                var_valid = ascii_ident
                var_invalid = [*unicode_ident, *digit, *special]
            case VarPattern.UNICODE_IDENTIFIER:
                var_valid = [*ascii_ident, *unicode_ident]
                var_invalid = [*digit, *special]
            case _ as other:
                assert_never(other)

        # escape characters
        esc_yes = [*((synt.dollar_escape,) or ())]
        esc_maybe = ['$', '\\']
        esc_no = [e for e in esc_maybe if e not in esc_yes]

        # expressions
        expr_var = [
            *((cls.expr_named,) if synt.named_form else ()),
            *((cls.expr_braced,) if synt.braced_form else ()),
        ]
        expr_lit = [e for e in (cls.expr_named, cls.expr_braced) if e not in expr_var]

        # return
        return cls(
            var_valid=var_valid,
            var_invalid=var_invalid,
            var_all=var_valid + var_invalid,
            esc_no=esc_no,
            esc_maybe=esc_maybe,
            expr_var=expr_var,
            expr_lit=expr_lit,
            expr_all=expr_var + expr_lit,
        )


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
    skip: Sequence[Callable[[ParameterSet], bool]] = ()
    more: Sequence[Callable[[], Iterable[ParameterSet]]] = ()
    # internals
    T: Templates | None = None

    def __post_init__(self):
        if self.T is None:
            self.T = Templates.from_syntax(self)

        for feature, invalid_values in (
            ('dollar_literal', (VarAction.USE_EMPTY,)),
            ('var_invalid', ()),
            ('var_unset', (VarAction.USE_ESCAPED,)),
        ):
            value = getattr(self, feature)
            if value in invalid_values:
                raise ValueError(f'{feature} value is invalid: {value}')

        if not self.dollar_escape and (
            self.dollar_literal == VarAction.USE_ESCAPED
            or self.var_invalid == VarAction.USE_ESCAPED
            or self.var_unset == VarAction.USE_ESCAPED
        ):
            raise ValueError('Invalid combination')

    def pytest_params(self) -> ParameterSet:
        # filter marked as skipped
        for param in self.elementary_cases():
            if any(must_skip(param) for must_skip in self.skip):
                param.marks = (pytest.mark.skip(),)
            yield param
        # add extras
        for gen in self.more:
            yield from gen()

    def elementary_cases(self) -> Iterable[ParameterSet]:
        T = self.T

        # named_form, braced_form
        for expr, name in product(T.expr_all, T.var_all):
            c = Case(expr.format(name), {name: 'value'})
            if expr in T.expr_var and name in T.var_valid:
                yield c.expect_value('value')
            elif expr == T.expr_braced and name in T.var_invalid:
                yield c.expect_action(self.var_invalid, self)
            else:
                # dollar literal followed by non-varname characters
                yield c.expect_action(self.dollar_literal, self)

        # dollar_escape
        for expr, name, esc in product(T.expr_all, T.var_all, T.esc_maybe):
            c = Case(f'{esc}{expr.format(name)}', {name: 'value'})
            if esc == self.dollar_escape:  # no_esc below
                yield c.expect_input()
            elif name in T.var_valid:  # var_invalid below
                yield c.expect_value(f'{esc}value')
            elif expr == T.expr_braced:
                yield c.expect_action(self.var_invalid, self)
            else:
                yield c.expect_action(self.dollar_literal, self)

        # dollar_literal (middle, last), w/wo escape


        # recursive depth

        # var_unset

        # var_case_sensitive
        for expr, name in product(T.expr_var, T.var_valid):
            for case in (
                Case(expr.format(name.upper()), {name.lower(): 'value'}),
                Case(expr.format(name.lower()), {name.upper(): 'value'}),
            ):
                if self.var_case_sensitive:
                    yield case.expect_action(self.var_unset, self)
                else:
                    yield case.expect_value('value')


@dataclass
class Case:
    input: str
    vars: dict[str, str]

    def expect_action(self, action: VarAction, synt: Syntax) -> pytest.param:
        match action:
            case VarAction.USE_ORIGINAL:
                return self.expect_input()
            case VarAction.USE_ESCAPED:
                if '$' in self.input:
                    return self.expect_value(
                        self.input.replace('$', f'{synt.dollar_escape}$', 1)
                    )
                else:
                    raise ValueError('not applicable here')
            case VarAction.USE_EMPTY:
                return self.expect_value('')
            case VarAction.ERROR:
                return self.expect_error()
            case _:
                assert_never(action)

    def expect_input(self):
        return pytest.param(self.input, self.vars, self.input)

    def expect_value(self, value: str):
        return pytest.param(self.input, self.vars, value)

    def expect_error(self) -> pytest.param:
        return pytest.param(self.input, self.vars, None)
