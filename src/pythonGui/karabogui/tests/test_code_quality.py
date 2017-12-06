import os.path as op
import subprocess

import karabogui


def test_code_quality_flake8():
    # Just run flake8 as if from the commandline
    command = ['flake8', op.dirname(op.abspath(karabogui.__file__))]
    subprocess.check_call(command)
