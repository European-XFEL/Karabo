import os
import subprocess

import karabo.interactive as interactive


BLACKLIST = ['__init__.py', 'convert_device_project.py']


def get_python_files():
    """Get all python files from this package"""
    common_dir = os.path.abspath(os.path.dirname(interactive.__file__))
    flake_check = []
    for dirpath, _, filenames in os.walk(common_dir):
        if dirpath.endswith('tests'):
            continue
        for fn in filenames:
            if os.path.splitext(fn)[-1].lower() == '.py' and \
                    fn not in BLACKLIST:
                path = os.path.join(dirpath, fn)
                flake_check.append(path)

    return flake_check


def test_code_quality_flake8():
    files = get_python_files()
    for py_file in files:
        command = ['flake8', os.path.abspath(py_file)]
        subprocess.check_call(command)
