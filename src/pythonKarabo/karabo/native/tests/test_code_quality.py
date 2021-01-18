import os.path as op
import subprocess
import sys
import unittest

import karabo.native as native


@unittest.skip(reason="New flake8 version")
def test_code_quality_flake8():
    # Just run flake8 as if from the commandline
    command = [sys.executable, '-m', 'flake8',
               op.dirname(op.abspath(native.__file__))]
    subprocess.check_call(command)
