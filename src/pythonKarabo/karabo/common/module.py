# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
