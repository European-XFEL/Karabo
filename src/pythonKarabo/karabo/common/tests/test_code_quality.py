# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op
import subprocess
import sys

import karabo.common as common


def test_code_quality_flake8():
    # Run flake8 as a module from the current Python interpreter.
    # Running 'flake8' directly assumes that flake8 is in the PATH,
    # which sometimes is not.
    command = [
        sys.executable,
        "-m",
        "flake8",
        op.dirname(op.abspath(common.__file__)),
    ]
    subprocess.check_call(command)
