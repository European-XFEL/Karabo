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
# flake8: noqa
import os.path as op

from karabo.testing.import_checker import check_for_star_imports


def test_bound_import():
    """ This is a smoke test that makes sure all of the bits available in the
    bound API are still there.
    """
    import karabo.bound


def test_bound_no_star_imports():
    """ Scream bloody murder if `import *` is ever added to the bound API
    module.

    If a star import is added to the bound API module, its contract
    is broken. We should avoid that.
    """
    try:
        import karabo.bound as bound_mod
    except ImportError:
        # We don't care about import errors here, just star imports.
        return

    check_for_star_imports(op.abspath(bound_mod.__file__))
