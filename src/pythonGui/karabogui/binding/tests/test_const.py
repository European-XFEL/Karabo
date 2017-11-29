from .. import const as constmod


def test_const_def():
    tuple_symbol = 'KARABO_SCHEMA_ATTRIBUTES'

    all_symbols = dir(constmod)
    all_symbols = [s for s in all_symbols if not s.startswith('__')]
    all_symbols.remove(tuple_symbol)
    all_values = {getattr(constmod, s) for s in all_symbols}

    # Make sure that all exported attribute names make it into the
    # KARABO_SCHEMA_ATTRIBUTES tuple.
    # NOTE: If this test fails, go to const.py and add entries to
    # KARABO_SCHEMA_ATTRIBUTES until this test passes
    assert all_values == set(getattr(constmod, tuple_symbol))
