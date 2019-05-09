import os
import subprocess


FILE_LIST = ['__init__.py', 'convert_device_project.py', 'deviceClient.py',
             'ideviceclient.py', 'ikarabo.py', 'jupyter_kernel.py',
             'karabo.py', 'scene2python.py', 'startkarabo.py', 'webserver.py']


def test_code_quality_flake8():
    command = ['flake8', *[os.path.abspath(py_file) for py_file in FILE_LIST]]
    subprocess.check_call(command)
