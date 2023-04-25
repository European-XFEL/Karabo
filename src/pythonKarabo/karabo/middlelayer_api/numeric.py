# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
