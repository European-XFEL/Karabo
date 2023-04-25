# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op
import sys
from unittest import TestCase, main

from jupyter_client.kernelspec import (
    find_kernel_specs, get_kernel_spec, install_kernel_spec)


class Tests(TestCase):
    def test_kernel(self):
        kernels = find_kernel_specs()
        # might be not installed if run in develop mode
        if 'Karabo' not in kernels:
            source_dir = op.join(
                op.dirname(__file__), op.pardir, "jupyter_spec")
            install_kernel_spec(
                source_dir, kernel_name='Karabo', prefix=sys.prefix)
        get_kernel_spec("Karabo")


if __name__ == "__main__":
    main()
