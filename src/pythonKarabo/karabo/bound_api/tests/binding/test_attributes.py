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
import pytest

from karabo.bound import (
    HashAttributes as Attributes, HashAttributesNode as Node, Types)


def test_constructor():
    # Can contruct only empty hash attributes
    ha = Attributes()
    assert len(ha) == 0
    assert ha.empty()
    # No args are accepted
    with pytest.raises(TypeError):
        ha = Attributes('a', 1)

    # Construction of HashAttributesNode is not possible in python
    # (but exceptions differ)
    with pytest.raises(TypeError):
        _ = Node()


def test_set():
    # Set attribute
    ha = Attributes()
    # BOOL
    ha.set('attr1', False)
    assert ha.getNode('attr1').getType() == Types.BOOL
    assert not ha.get('attr1')
    # INT32
    ha.set('attr2', 12)
    assert ha.getNode('attr2').getType() == Types.INT32
    assert ha.get('attr2') == 12
    # INT64
    ha.set('attr3', 333333333333)
    assert ha.getNode('attr3').getType() == Types.INT64
    assert ha.get('attr3') == 333333333333
    # DOUBLE
    ha.set('attr4', 3.14159265359)
    assert ha.getNode('attr4').getType() == Types.DOUBLE
    assert ha.get('attr4') == 3.14159265359
    # COMPLEX_DOUBLE
    ha.set('attr5', complex(12., 42.))
    assert ha.getNode('attr5').getType() == Types.COMPLEX_DOUBLE
    assert ha.get('attr5') == complex(12., 42.)
    # STRING
    ha.set('attr6', 'abrakadabra')
    assert ha.getNode('attr6').getType() == Types.STRING
    assert ha.get('attr6') == 'abrakadabra'

    # BOOL
    ha['attr7'] = True
    assert ha.getNode('attr7').getType() == Types.BOOL
    assert ha['attr7']
    # INT32
    ha['attr8'] = 42
    assert ha.getNode('attr8').getType(), Types.INT32
    assert ha['attr8'] == 42
    # INT64
    ha['attr9'] = -123456789012
    assert ha.getNode('attr9').getType() == Types.INT64
    assert ha['attr9'] == -123456789012
    # DOUBLE
    ha['attr10'] = 2.718281828
    assert ha.getNode('attr10').getType() == Types.DOUBLE
    assert ha['attr10'] == 2.718281828
    # COMPLEX_DOUBLE
    ha['attr11'] = complex(9999.12, -65432.03)
    assert ha.getNode('attr11').getType() == Types.COMPLEX_DOUBLE
    # STRING
    ha['attr12'] = 'some string'
    assert ha.getNode('attr12').getType() == Types.STRING
    # VECTOR_BOOL
    ha['attr13'] = [True, False, False, True, True]
    assert ha.getNode('attr13').getType() == Types.VECTOR_BOOL
    # VECTOR_INT32
    ha['attr14'] = [1, 2, 3, 4, 5]
    assert ha.getNode('attr14').getType() == Types.VECTOR_INT32
    # VECTOR_INT64
    ha['attr15'] = [12, 13, 1444444444444, -2, 0]
    assert ha.getNode('attr15').getType() == Types.VECTOR_INT64
    # VECTOR_DOUBLE
    ha['attr16'] = [1.11111, 2.222222, 3.333333]
    assert ha.getNode('attr16').getType() == Types.VECTOR_DOUBLE
    # VECTOR_COMPLEX_DOUBLE
    ha['attr17'] = [complex(1.0, -5), complex(-2., 188)]
    assert ha.getNode('attr17').getType() == Types.VECTOR_COMPLEX_DOUBLE
    assert ha.getType('attr17') == Types.VECTOR_COMPLEX_DOUBLE
    assert ha.isType('attr17', Types.VECTOR_COMPLEX_DOUBLE)
    assert ha.isType('attr17', "VECTOR_COMPLEX_DOUBLE")
    ha.erase('attr17')
    ha['attr17'] = [complex(1.0, -5), complex(-2., 188)]
    del ha['attr17']

    # VECTOR_STRING
    ha['attr18'] = ['aaaa', 'bbb', 'cccccccc', 'www']
    assert ha.getNode('attr18').getType() == Types.VECTOR_STRING
    assert len(ha) == 17
    assert 'attr18' in ha
    assert 'attr17' not in ha
    assert not ha.empty()
    # Get reference to node
    node6 = ha.getNode('attr6')
    assert isinstance(node6, Node)
    assert node6.getKey() == 'attr6'
    assert node6.getType() == Types.STRING
    assert node6.getValue() == 'abrakadabra'
    assert ha.getType('attr1') == Types.BOOL
    assert ha.isType('attr1', Types.BOOL)
    assert ha.isType('attr1', "BOOL")
    assert ha.getType('attr2') == Types.INT32
    assert ha.isType('attr2', Types.INT32)
    assert ha.isType('attr2', "INT32")
    assert ha.getType('attr3') == Types.INT64
    assert ha.isType('attr3', Types.INT64)
    assert ha.isType('attr3', "INT64")
    assert ha.getType('attr4') == Types.DOUBLE
    assert ha.isType('attr4', Types.DOUBLE)
    assert ha.isType('attr4', "DOUBLE")
    assert ha.getType('attr5') == Types.COMPLEX_DOUBLE
    assert ha.isType('attr5', Types.COMPLEX_DOUBLE)
    assert ha.isType('attr5', "COMPLEX_DOUBLE")
    assert ha.getType('attr6') == Types.STRING
    assert ha.isType('attr6', Types.STRING)
    assert ha.isType('attr6', "STRING")
    assert ha.getType('attr13') == Types.VECTOR_BOOL
    assert ha.isType('attr13', Types.VECTOR_BOOL)
    assert ha.isType('attr13', "VECTOR_BOOL")
    assert ha.getType('attr14') == Types.VECTOR_INT32
    assert ha.isType('attr14', Types.VECTOR_INT32)
    assert ha.isType('attr14', "VECTOR_INT32")
    assert ha.getType('attr15') == Types.VECTOR_INT64
    assert ha.isType('attr15', Types.VECTOR_INT64)
    assert ha.isType('attr15', "VECTOR_INT64")
    assert ha.getType('attr16') == Types.VECTOR_DOUBLE
    assert ha.isType('attr16', Types.VECTOR_DOUBLE)
    assert ha.isType('attr16', "VECTOR_DOUBLE")
    assert ha.getType('attr18') == Types.VECTOR_STRING
    assert ha.isType('attr18', Types.VECTOR_STRING)
    assert ha.isType('attr18', "VECTOR_STRING")
    assert not ha.isType('attr15', "INT64")

    ha.clear()
    assert (ha.empty())


def test_iteration():
    ha = Attributes()
    ha['a'] = 8100000000000000
    ha['b'] = True
    ha['c'] = 2.333333333
    ha['s'] = 'Hello'

    it = iter(ha)
    n = next(it)
    assert isinstance(n, Node)
    assert n.getKey() == 'a'
    assert n.getType() == Types.INT64
    assert n.getValue() == 8100000000000000
    assert n.getValueAs(Types.STRING) == '8100000000000000'

    n = next(it)
    assert isinstance(n, Node)
    assert n.getKey() == 'b'
    assert n.getType() == Types.BOOL
    assert n.getValue() is True

    n = next(it)
    assert n.getKey() == 'c'
    assert n.getType() == Types.DOUBLE

    n = next(it)
    assert n.getKey() == 's'
    assert n.getType() == Types.STRING
    assert n.getValue() == 'Hello'

    with pytest.raises(StopIteration):
        n = next(it)


def test_changetype():
    ha = Attributes()
    ha['a'] = 65

    def current_type_is(reftype):
        assert ha.getType('a') == reftype

    current_type_is(Types.INT32)
    # Get as INT64
    assert ha.getAs('a', "INT64"), 65
    # Get as INT16
    assert ha.getAs('a', "INT16"), 65
    # Get as INT8
    assert ha.getAs('a', "INT8"), 65
    # Get as DOUBLE
    assert ha.getAs('a', "DOUBLE"), 65.0
    # Attempt to get as BOOL: Cast Exception
    with pytest.raises(RuntimeError):
        assert (ha.getAs('a', "BOOL"))
    # Get as COMPLEX_DOUBLE
    assert ha.getAs('a', "COMPLEX_DOUBLE") == complex(65, 0)
    # Get as STRING
    assert ha.getAs('a', "STRING") == '65'

    # Change the type ...
    current_type_is(Types.INT32)
    assert ha['a'] == 65
    # INT64
    ha.getNode('a').setType("INT64")
    current_type_is(Types.INT64)
    assert ha['a'] == 65
    # INT16
    ha.getNode('a').setType("INT16")
    current_type_is(Types.INT16)
    assert ha['a'] == 65
    # INT8
    ha.getNode('a').setType("INT8")
    current_type_is(Types.INT8)
    assert ha['a'] == 65
    # DOUBLE
    ha.getNode('a').setType("DOUBLE")
    current_type_is(Types.DOUBLE)
    assert ha['a'] == 65.0
    # COMPLEX_DOUBLE
    ha.getNode('a').setType("COMPLEX_DOUBLE")
    current_type_is(Types.COMPLEX_DOUBLE)
    assert ha['a'] == complex(65, 0)  # (65+0j)
    # STRING
    ha.getNode('a').setType("STRING")
    current_type_is(Types.STRING)
    assert ha['a'] == '(65,0)'
