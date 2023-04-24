# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# flake8: noqa
import os.path as op

from karabo.testing.import_checker import check_for_star_imports


def test_bound_api_import():
    """ This is a smoke test that makes sure all of the bits available in the
    bound API are still there.
    """
    import karabo.bound


def test_bound_api_no_star_imports():
    """ Scream bloody murder if `import *` is ever added to the bound API
    module.

    If a star import is added to the bound API module, its contract
    is broken. We should avoid that.
    """
    try:
        import karabo.bound as bound_api_mod
    except ImportError:
        # We don't care about import errors here, just star imports.
        return

    check_for_star_imports(op.abspath(bound_api_mod.__file__))
