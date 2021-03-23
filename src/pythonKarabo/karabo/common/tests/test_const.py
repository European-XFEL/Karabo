from inspect import isfunction
from karabo.common import const as constmod


def test_schema_attributes_def():
    tuple_symbol = 'KARABO_SCHEMA_ATTRIBUTES'
    ignored_symbols = ('KARABO_EDITABLE_ATTRIBUTES',
                       'KARABO_RUNTIME_ATTRIBUTES_MDL',
                       'KARABO_SCHEMA_DEFAULT_SCENE')

    all_symbols = dir(constmod)
    all_symbols = [s for s in all_symbols if not s.startswith('__')
                   and not s.startswith('KARABO_TYPE')]
    # Remove all functions
    all_symbols = [s for s in all_symbols if not isfunction(
        getattr(constmod, s))]
    all_symbols.remove(tuple_symbol)
    for symbol in ignored_symbols:
        all_symbols.remove(symbol)
    all_values = {getattr(constmod, s) for s in all_symbols}

    # Make sure that all exported attribute names make it into the
    # KARABO_SCHEMA_ATTRIBUTES tuple.
    # NOTE: If this test fails, go to const.py and add entries to
    # KARABO_SCHEMA_ATTRIBUTES until this test passes
    assert all_values == set(getattr(constmod, tuple_symbol))


def test_schema_types():
    """Make sure that all the schema types are appearing in the tuple summary
    """
    tuple_symbol = 'KARABO_TYPES'
    all_symbols = dir(constmod)
    all_symbols = [s for s in all_symbols if s.startswith('KARABO_TYPE')]
    all_symbols.remove(tuple_symbol)

    all_values = {getattr(constmod, s) for s in all_symbols}
    assert all_values == set(getattr(constmod, tuple_symbol))
