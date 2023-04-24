# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op
import subprocess
import sys

import karabo.interactive as interactive_pkg


def test_code_quality_flake8():
    # Just run flake8 as if from the commandline
    command = [sys.executable, '-m', 'flake8',
               op.dirname(op.abspath(interactive_pkg.__file__))]
    subprocess.check_call(command)
