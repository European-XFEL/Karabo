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
import sys
import types


def create_module(name, *symbols, docstring=""):
    """Create a module, fill it with symbols, and "import" it into the system
    module namespace.

    This is useful for defining "virtual" submodules of things like API modules
    """
    assert name not in sys.modules

    mod = types.ModuleType(name, docstring)
    for sym in symbols:
        sym_name = sym.__name__
        if isinstance(sym, types.ModuleType):
            # watch out for module objects!
            sym_name = sym_name.split(".")[-1]
        setattr(mod, sym_name, sym)

    sys.modules[name] = mod
    return mod
