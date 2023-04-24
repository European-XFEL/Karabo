# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op
import subprocess

import karabo.bound_api as bound_api


def test_code_quality_flake8():
    # Just run flake8 as if from the commandline
    command = ['flake8', op.dirname(op.abspath(bound_api.__file__))]
    subprocess.check_call(command)
