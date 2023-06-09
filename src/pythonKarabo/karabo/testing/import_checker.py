# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import ast
import os
import os.path as op


def _get_ast(path):
    """ Get an ast.AST object for the specified file.
    """
    with open(path, 'rb') as fp:
        return compile(fp.read(), path, "exec", ast.PyCF_ONLY_AST)


def check_for_disallowed_module_imports(forbidden_module, path):
    """ Check a source file for imports from ``forbidden_module``.

    NOTE: This is only useful for relative imports if, by traversing up to
    the level of the import's base package, the search module's name gets
    pulled into the module name. See the examples below.

    Examples:
      1) Relative from two levels up
        file: base/foo/bar/baz/qux.py
        import: from ..other import blah
        module name: bar.other
      2) Relative from two levels up and one level down
        file: base/foo/bar/baz/qux.py
        import: from ..baz_neighbor.child import blah
        module name: bar.baz_neighbor.child

    Notice that neither module contains 'base.foo', because the AST won't tell
    us if those directories are proper Python packages.
    """
    def _get_name_from_level(level):
        parts = op.dirname(path).split(os.sep)
        return parts[-level]

    tree = _get_ast(path)
    warning_msg = ('Imports are not allowed from "{}"'
                   ' in this module! ({})').format(forbidden_module, path)
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for imp in node.names:
                assert forbidden_module not in imp.name, warning_msg
        elif isinstance(node, ast.ImportFrom):
            module = node.module
            if node.level > 0:
                base = _get_name_from_level(node.level)
                module = f'{base}.{module}'
            assert forbidden_module not in module, warning_msg


def check_for_star_imports(path):
    """ Check a source file for "import *" usage.
    """
    tree = _get_ast(path)
    warning_msg = ('Star imports are not allowed in this module!'
                   ' ({})').format(path)
    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom):
            for alias in node.names:
                assert alias.name != '*', warning_msg


def run_checker_on_package(package_mod, checker_func, skip_tests=True):
    """ Run an AST checker function on some package recursively
    """
    common_dir = op.abspath(op.dirname(package_mod.__file__))
    for dirpath, _, filenames in os.walk(common_dir):
        # (maybe) skip test modules!
        if skip_tests and dirpath.endswith('tests'):
            continue

        for fn in filenames:
            if op.splitext(fn)[-1].lower() == '.py':
                path = op.join(dirpath, fn)
                checker_func(path)
