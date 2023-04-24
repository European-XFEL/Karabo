# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op

from karabo.testing.import_checker import check_for_star_imports


def test_macro_api_no_star_imports():
    """ Scream bloody murder if `import *` is ever added to the Macro API.
    """
    try:
        import karabo.macros as macro_api_mod
    except ImportError:
        # We don't care about import errors here, just star imports.
        return

    check_for_star_imports(op.abspath(macro_api_mod.__file__))
