import os.path as op

from karabo.testing.import_checker import check_for_star_imports


def test_device_api_2_import():
    """ This is a smoke test that makes sure all of the bits available in API 2
    are still there.
    """
    import karabo.api_2


def test_device_api_2_no_star_imports():
    """ Scream bloody murder if `import *` is ever added to the device API
    module.

    If a star import is added to the device API module, its contract
    is broken. We should avoid that.
    """
    try:
        import karabo.api_2 as api_mod
    except ImportError:
        # We don't care about import errors here, just star imports.
        return

    check_for_star_imports(op.abspath(api_mod.__file__))
