import ast


def _iter_nodes(node):
    """ Iterate over all of the nodes in an AST.
    """
    for name in node._fields:
        field = getattr(node, name, None)
        if isinstance(field, ast.AST):
            yield field
        elif isinstance(field, list):
            for item in field:
                yield item


def check_for_star_imports(path):
    """ Check a source file for "import *" usage.
    """
    with open(path, 'r') as fp:
        codestring = fp.read()

    warning_msg = 'Star imports are not allowed in this module!'
    tree = compile(codestring, path, "exec", ast.PyCF_ONLY_AST)

    for node in _iter_nodes(tree):
        if isinstance(node, ast.ImportFrom):
            for alias in node.names:
                assert alias.name != '*', warning_msg
