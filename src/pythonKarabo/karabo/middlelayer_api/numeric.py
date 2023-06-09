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
# flake8: noqa: F401
"""
Unit-aware replacements for :py:mod:`numpy` functions
=====================================================

Most :py:mod:`numpy` functions (namely: ufuncs) support Karabo values
out-of-the-box.
Some, however, do not. Karabo defines its own drop-in replacements for those
functions in the :py:mod:`karabo.numeric` module. These are:

.. function:: karabo.numeric.linspace

    See :py:func:`numpy.linspace`

.. function:: karabo.numeric.dot

    See :py:func:`numpy.dot`

.. function:: karabo.numeric.cross

    See :py:func:`numpy.cross`
"""
from .unitutil import cross, dot, linspace
