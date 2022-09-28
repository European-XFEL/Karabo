import os.path as op
import subprocess
import sys

import karabo.middlelayer_api as middlelayer_api


def test_code_quality_flake8():
    # Just run flake8 as if from the commandline
    command = [sys.executable, '-m', 'flake8',
               op.dirname(op.abspath(middlelayer_api.__file__))]
    subprocess.check_call(command)
