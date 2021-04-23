import ast


def validate_macro(code):
    """Validate a macro source code several use cases
    """
    # Get ast.AST object for code first
    tree = compile(code, "MacroSanityCheck", "exec", ast.PyCF_ONLY_AST)
    ret = []
    # Check if we are using forbidden imports
    lines = _has_imports(tree, "time", "sleep")
    if lines:
        ret.extend(lines)

    # - Update is the `update` method of a device
    # - register is a function of a `Configurable` to register descriptors
    # - cancel is the native cancel function of a macro
    # - clear_namespace is the gui binding clear command
    lines = _has_methods(tree, "update", "clear_namespace", "register",
                         "cancel")
    if lines:
        ret.extend(lines)

    return ret


def _has_imports(tree, module, func):
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
        if (isinstance(node, ast.Expr) and isinstance(node.value, ast.Call)
                and isinstance(node.value.func, ast.Attribute)
                and isinstance(node.value.func.value, ast.Name)):
            for alias in as_names:
                if (node.value.func.value.id == alias
                        and node.value.func.attr == func):
                    return "Found {}.{} in line {}".format(
                        alias, func, node.lineno)

    def _is_module_call(node, module, func):
        """Check if the module function is called within the code"""
        if (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                and isinstance(node.func.value, ast.Name)):
            if node.func.value.id == module and node.func.attr == func:
                return "Found {}.{} in line {}".format(
                    module, func, node.lineno)
        return None

    def _is_imported(node, module, func):
        """Check if the module and function is already directly imported"""
        if isinstance(node, ast.ImportFrom):
            if node.module == module:
                for alias in node.names:
                    if alias.name == func:
                        return "Found {}.{} in line {}".format(
                            module, func, node.lineno)
        elif isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name == "{}.{}".format(module, func):
                    return "Found {}.{} in line {}".format(
                        module, func, node.lineno)
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


def _has_methods(tree, *methods):
    reports = []
    for node in ast.walk(tree):
        if isinstance(node, (ast.AsyncFunctionDef, ast.FunctionDef)):
            for method in methods:
                if node.name == method:
                    reports.append("Found forbidden method `{}` in "
                                   "line {}".format(method, node.lineno))
    return reports
