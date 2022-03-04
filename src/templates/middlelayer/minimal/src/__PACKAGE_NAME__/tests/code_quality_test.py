import os.path as op
import os
import subprocess

import __PACKAGE_NAME__


BLACKLIST = ['setup.py', '__init__.py']


def get_python_files():
    """Get all python files from this package
    """
    common_dir = op.abspath(op.dirname(__PACKAGE_NAME__.__file__))
    flake_check = []
    for dirpath, _, filenames in os.walk(common_dir):
        for fn in filenames:
            if op.splitext(fn)[-1].lower() == '.py' and fn not in BLACKLIST:
                path = op.join(dirpath, fn)
                flake_check.append(path)

    return flake_check


def test_code_quality_flake8():
    files = get_python_files()
    command = ['flake8', *[op.abspath(py_file) for py_file in files]]
    subprocess.check_call(command)
