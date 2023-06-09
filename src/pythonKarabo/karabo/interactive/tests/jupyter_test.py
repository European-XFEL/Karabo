# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
