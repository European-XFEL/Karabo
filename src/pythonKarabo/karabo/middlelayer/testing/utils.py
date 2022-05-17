import ast
import os
from pathlib import Path


def get_ast_objects(package, ignore=[]):
    """Get all ast objects from a specified package

    :param ignore: list of filenames that are ignored in the lookup
    """
    def _get_ast(path):
        """Get an ast.AST object for the specified file"""
        with open(path, 'rb') as fp:
            return compile(fp.read(), path, "exec", ast.PyCF_ONLY_AST)

    common_dir = str(Path(package.__file__).parent)
    ast_objects = []
    for dirpath, _, filenames in os.walk(common_dir):
        for fn in filenames:
            if Path(fn).suffix == ".py" and fn not in ignore:
                path = str(Path(dirpath).joinpath(fn))
                ast_objects.append(_get_ast(path))
    return ast_objects
