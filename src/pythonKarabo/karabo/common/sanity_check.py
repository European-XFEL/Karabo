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


def validate_macro(code):
    """Validate a macro source code several use cases"""
    # Get ast.AST object for code first
    tree = compile(code, "MacroSanityCheck", "exec", ast.PyCF_ONLY_AST)
    ret = []
    # Check if we are using forbidden imports
    lines = has_imports(tree, "time", "sleep")
    if lines:
        ret.extend(lines)

    # - Update is the `update` method of a device
    # - register is a function of a `Configurable` to register descriptors
    # - cancel is the native cancel function of a macro
    # - clear_namespace is the gui binding clear command
    lines = has_methods(
        tree, "update", "clear_namespace", "register", "cancel"
    )
    if lines:
        ret.extend(lines)

    # filter out `except` handlers catching base exception to launching
    # unstoppable macros on the macro server.
    lines = has_base_exceptions(tree)
    if lines:
        ret.extend(lines)

    # Make sure there are no sub module imports
    for mod in ["karabo.middlelayer"]:
        lines = has_sub_imports(tree, mod)
        if lines:
            ret.extend(lines)
    return ret


def has_imports(tree, module, func):
    """Check an ast tree for a forbidden import and its usage

    :param tree: the code tree as ast object
    :param module: the string of the module, e.g. ``time``
    :param func: the function of the module, e.g. ``sleep``
    """

    def _retrieve_alias(tree, module):
        """Checks if the module we are interested in is ``imported as``"""
        as_names = []
        for node in ast.walk(tree):
            if isinstance(node, ast.alias):
                if node.name == module:
                    as_names.append(node.asname)
        return as_names

    def _is_alias_call(node, as_names, func):
        """Check if an aliased module is used

        This function requires a valid definition of alias names in
        ``as_names``.
        """
        if (
            isinstance(node, ast.Expr)
            and isinstance(node.value, ast.Call)
            and isinstance(node.value.func, ast.Attribute)
            and isinstance(node.value.func.value, ast.Name)
        ):
            for alias in as_names:
                if (
                    node.value.func.value.id == alias
                    and node.value.func.attr == func
                ):
                    return "Found {}.{} in line {}".format(
                        alias, func, node.lineno
                    )

    def _is_module_call(node, module, func):
        """Check if the module function is called within the code"""
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name)
        ):
            if node.func.value.id == module and node.func.attr == func:
                return "Found {}.{} in line {}".format(
                    module, func, node.lineno
                )
        return None

    def _is_imported(node, module, func):
        """Check if the module and function is already directly imported"""
        if isinstance(node, ast.ImportFrom):
            if node.module == module:
                for alias in node.names:
                    if alias.name == func:
                        return "Found {}.{} in line {}".format(
                            module, func, node.lineno
                        )
        elif isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name == f"{module}.{func}":
                    return "Found {}.{} in line {}".format(
                        module, func, node.lineno
                    )
        return None

    as_names = _retrieve_alias(tree, module)
    reports = []
    for node in ast.walk(tree):
        found = _is_alias_call(node, as_names, func)
        if found is not None:
            reports.append(found)
        found = _is_module_call(node, module, func)
        if found is not None:
            reports.append(found)
        found = _is_imported(node, module, func)
        if found is not None:
            reports.append(found)

    return reports


def has_methods(tree, *methods):
    reports = []
    for node in ast.walk(tree):
        if isinstance(node, (ast.AsyncFunctionDef, ast.FunctionDef)):
            for method in methods:
                if node.name == method:
                    reports.append(
                        "Found forbidden method `{}` in "
                        "line {}".format(method, node.lineno)
                    )
    return reports


def has_base_exceptions(tree):
    reports = []
    for node in ast.walk(tree):
        if isinstance(node, ast.ExceptHandler):
            # catch the `except:`
            explanation = (
                " Use `except Exception as e:`"
                " or catch the specific exceptions."
                " This behaviour will likely result in a macro"
                " that cannot be stopped"
            )
            if node.type is None:
                reports.append(
                    f"Found `except:` clause in line {node.lineno}:"
                    " {explanation}"
                )
            elif node.type.id == "BaseException":
                # catch the `except BaseException:`
                # XXX: the CancelledError is dangerous as well,
                # but we chose to allow it here
                reports.append(
                    f"Found `except {node.type.id}:`"
                    f" clause in line {node.lineno}: {explanation}"
                )
    return reports


def has_sub_imports(tree, global_module, ignore=[]):

    reports = []

    def _check_sub_imports(module):
        """Check for sub imports of the module"""
        if global_module in module:
            # Strip global_module, if something is left, we have a subimport
            sub = module.strip(global_module)
            return False if sub else True
        return True

    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom):
            module = node.module
            if ignore and module in ignore or module is None:
                continue
            if not _check_sub_imports(module):
                warning_msg = (
                    f'Sub imports are not allowed from "{global_module}". '
                    f'The import is from module {module}!')
                reports.append(warning_msg)

    return reports
