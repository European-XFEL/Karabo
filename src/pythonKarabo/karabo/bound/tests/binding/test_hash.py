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
import copy

import numpy as np
import pytest

from karabo.bound import (
    Hash, HashMergePolicy, HashNode as Node, Types, VectorHash, fullyEqual,
    similar)

# Test "clear", "empty", "getKeys", "keys", "getValues", "values",
#      "getPaths", "paths", "set", "__setitem__", "setAs", "get",
#      "__getitem__", "has", "__contains__", "erase", "__iter__",
#      "__delitem__", "erasePath", "__len__", "__bool__", "getAs",
#      "getType", "merge", "__iadd__", "__isub__", "similar",
#      "fullyEqual", "isType", "flatten", "unflatten", "find",
#      "setNode", "getNode", "hasAttribute", "getAttribute",
#      "getAttributeAs", "getAttributes", "copyAttributes",
#      "setAttribute", "setAttributes", "__copy__",
#      "__deepcopy__"


def test_constructor():
    h = Hash()

    # Test "__len__"
    assert len(h) == 0

    # Test "empty"
    assert h.empty()

    # Test "__bool__"
    assert not h
    # It is allowed any number of key/value pairs to be defined
    h = Hash('a', 1, 'b', 2, 'c', 3, 'd', 4, 'e', 5, 'f', 6)
    assert h
    # For further populating, use set ...
    h.set('g', 7)
    assert len(h) == 7
    assert not h.empty()

    # Test "clear"
    h.clear()
    assert h.empty()

    # Test constructors
    # There are no restrictions in number of key/value pairs
    h = Hash('a', 1, 'b', 2, 'c', 3, 'd', 4, 'e', 5, 'f', 6, 'g', 7, 'h', 8)
    assert h
    # Define python dictionary...
    d = {'a': 1, 'b': {'c': 12}, 'c': {'e': {'f': "abrakadabra"}}}
    h = Hash(d)
    assert h
    assert "c.e.f" in h
    assert h.get('b.c') == 12
    # Cannot construct Node object in Python ...
    with pytest.raises(TypeError):
        print(Node())


def test_getset():
    # Test "get" .... default value
    h = Hash()
    assert h.get("missing") is None
    assert h.get("missing", default="value") == "value"

    # Test "set"
    h.set('a', "some text")
    h.set('b.c.d.e.f', 2.718281828)

    # Test "__setitem__"
    h['w.r.a.p'] = 123456789999
    h['c.c.c.c'] = True
    h['p.e.r.f'] = [1.1, 2.2, 3.3]

    # Test "get"
    assert h.get('a') == "some text"
    assert h.get('p.e.r.f') == [1.1, 2.2, 3.3]

    # Test "__getItem__"
    assert h['b.c.d.e.f'] == 2.718281828
    assert h['c.c.c.c'] is True

    # Test "getType"
    assert h.getType('b.c.d.e.f') == Types.DOUBLE
    assert h.getType('p.e.r.f') == Types.VECTOR_DOUBLE

    # Test "isType"
    assert h.isType('a', Types.STRING)
    assert h.isType('c.c.c.c', Types.BOOL)
    assert h.isType('a', "STRING")
    assert h.isType('c.c.c.c', "BOOL")

    # Test "has"
    assert h.has('p.e.r.f')
    assert not h.has("miss.ing")

    # Test "keys"
    assert h.keys() == ['a', 'b', 'w', 'c', 'p']

    # Test "paths"
    assert h.paths() == ['a', 'b.c.d.e.f', 'w.r.a.p', 'c.c.c.c', 'p.e.r.f']
    # Test "getKeys"
    assert h.getKeys() == ['a', 'b', 'w', 'c', 'p']

    # Test "getPaths"
    assert h.getPaths() == ['a', 'b.c.d.e.f', 'w.r.a.p', 'c.c.c.c', 'p.e.r.f']
    # Test C++-ish "getKeys"
    res = []
    h.getKeys(res)
    assert res == ['a', 'b', 'w', 'c', 'p']

    # Test C++-ish "getPaths"
    res = []
    h.getPaths(res)
    assert res == ['a', 'b.c.d.e.f', 'w.r.a.p', 'c.c.c.c', 'p.e.r.f']
    # Test "values"
    assert h.values() == ["some text",
                          Hash('c.d.e.f', 2.718281828),
                          Hash('r.a.p', 123456789999),
                          Hash('c.c.c', True),
                          Hash('e.r.f', [1.1, 2.2, 3.3])]
    # Test "getValues"
    assert h.getValues() == ["some text",
                             Hash('c.d.e.f', 2.718281828),
                             Hash('r.a.p', 123456789999),
                             Hash('c.c.c', True),
                             Hash('e.r.f', [1.1, 2.2, 3.3])]

    # Test "flatten"
    g = Hash()
    h.flatten(g)
    assert g.keys() == ['a', 'b.c.d.e.f', 'w.r.a.p',
                        'c.c.c.c', 'p.e.r.f']
    assert g.keys() == g.paths()
    # ERROR: assert 'b.c.d.e.f' in g)
    assert g.has('b.c.d.e.f', '/')
    assert not g.has('b.c.d', '/')
    assert g.values() == ["some text", 2.718281828,
                          123456789999, True, [1.1, 2.2, 3.3]
                          ]
    # NOTE: flatten Hash has keys containing '.' considered as default
    # separator.  The 'get with default separator will fail.  To get
    # the value, one can use another separator like
    assert g.get('b.c.d.e.f', '/') == 2.718281828
    assert g.get('w.r.a.p', '|') == 123456789999

    # Test "unflatten"
    f = Hash()
    g.unflatten(f)
    assert f == h  # '==' is 'similar'
    assert similar(f, h)
    assert fullyEqual(f, h)
    g.clear()
    assert not g

    # Test "erase"
    f.erase('b.c.d.e')
    assert 'b' in f
    assert 'b.c.d.e' not in f
    assert 'b.c.d' in f
    assert f.getType('b.c.d') == Types.HASH
    assert f['b.c.d'] == Hash()

    # Test "erasePath"
    assert f['p.e.r.f'] == [1.1, 2.2, 3.3]
    f.erasePath('p.e.r')
    assert 'p.e.r' not in f
    assert 'p.e' not in f
    assert 'p' not in f

    # Test "__delitem__"
    del f['w.r']
    assert 'w.r' not in f
    assert 'w' in f
    assert f['w'] == Hash()
    assert h['w'] == Hash('r.a.p', 123456789999)

    del f

    # Test "__iter__"
    assert len(h) == 5
    it = iter(h)

    assert h['a'] == "some text"
    n1 = next(it)
    assert isinstance(n1, Node)
    assert n1.getKey() == 'a'
    assert n1.getValue() == 'some text'
    assert n1.getType() == Types.STRING

    assert h['b.c.d.e.f'] == 2.718281828
    n2 = next(it)
    assert isinstance(n2, Node)
    assert n2.getKey() == 'b'
    assert n2.getValue(), Hash('c.d.e.f', 2.718281828)
    assert n2.getType() == Types.HASH

    assert h['w.r.a.p'] == 123456789999
    n3 = next(it)
    assert isinstance(n3, Node)
    assert n3.getKey() == 'w'
    assert n3.getValue() == Hash('r.a.p', 123456789999)
    assert n3.getType() == Types.HASH

    assert h['c.c.c.c'] is True
    n4 = next(it)
    assert isinstance(n4, Node)
    assert n4.getKey() == 'c'
    assert n4.getValue() == Hash('c.c.c', True)
    assert n4.getType() == Types.HASH

    assert h['p.e.r.f'] == [1.1, 2.2, 3.3]
    n5 = next(it)
    assert isinstance(n5, Node)
    assert n5.getKey() == 'p'
    assert n5.getValue() == Hash('e.r.f', [1.1, 2.2, 3.3])
    assert n5.getType() == Types.HASH

    with pytest.raises(StopIteration):
        next(it)

    # Test "find"
    n = h.find('b.c.d')
    assert n
    assert n.getKey() == 'd'
    n = h.find('b.c.x')
    assert not n

    # Test "get" and/or "__getitem__" while pointing to the middle
    # of the parent tree 'h'
    # The resulting subtree behaves like a reference
    # Source "parent" tree is ...
    assert h['b.c.d.e.f'] == 2.718281828
    assert h.getType('b.c.d.e.f') == Types.DOUBLE
    assert h.getType('b.c.d') == Types.HASH
    # Test "__getitem__"
    g = h['b.c.d']  # reference to the subtree
    assert g['e.f'] == 2.718281828
    assert g.getType('e.f') == Types.DOUBLE
    # set new value via subtree ref.
    g['e.f'] = 'new long string'
    # the type is changed ...
    assert g.getType('e.f') == Types.STRING
    # Check that this is real reference, not a copy...
    assert h['b.c.d.e.f'] == 'new long string'
    assert h.getType('b.c.d.e.f') == Types.STRING

    # Reference from reference?
    # parent tree ... the owner
    h = Hash('a.b.c.d.e.f', 1)
    assert h['a.b.c.d.e.f'] == 1
    # reference to the node 'a.b.c' ... the proxy
    g = h['a.b.c']
    g['d.e.f'] = 55
    assert h['a.b.c.d.e.f'] == 55
    # reference to the node 'd.e' ... the proxy
    t = g['d.e']
    t['f'] = 88
    assert g['d.e.f'] == 88
    assert h['a.b.c.d.e.f'] == 88
    # drop the parent Hash in Python
    del h
    # Attempt to access 'g' or 't' gives possible despite 'h' is gone
    # (lifetime extended via 'keep_alive<...>()' magic)
    assert g['d.e.f'] == 88
    assert t['f'] == 88

    h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))
    assert h["a[0].a"] == 1, "Value should be 1"
    assert h["a[1].a"] == 1, "Value should be 1"
    assert h["a"][0]["a"] == 1, "Value should be 1"
    assert h["a"][1]["a"] == 1, "Value should be 1"
    # destroy parent Hash in C++
    h.clear()
    del g  # the subtree is invalidated. SEGFAULT if accessed.
    assert h.empty()

    # Comparison of 2 Hashes using operators: "==" and "!="
    h = Hash('a.b.c', True, 'x.y.z', [1, 2, 3])
    g = copy.copy(h)
    assert h == g
    # Change some values in the copy
    g['a.b.c'] = False
    g['x.y.z'] = [9, 8, 7, 6, 5]
    # BTW, check that above `copy` is a real copy and not a reference
    assert h['a.b.c'] is True
    assert h['x.y.z'] == [1, 2, 3]

    # Changing the value of leaf does not change the equality because
    # in C++ the operators '==' and '!=' use call (erroneously) to
    # 'similar(...)' method which checks only the "structure" equality
    # and does not compare the leaves ...
    assert h == g
    # But if the structure is changed ...
    h['a.b.c'] = 12  # the type is changed from BOOL to INT32
    assert h != g
    # ... or some new key/value was added ...
    g = copy.copy(h)  # restore equality
    g['h.e.l.p'] = 2.78
    assert h != g

    # Test "getNode"
    h = Hash('a', 12)
    n = h.getNode('a')
    assert n.getKey() == 'a'
    assert n.getValue() == 12
    assert n.getType() == Types.INT32

    # Test "setNode"
    f = Hash('b', "text")
    f.setNode(n)
    assert 'b' in f
    assert 'a' in f
    assert f['a'] == 12
    assert n.getValue() == 12
    assert f.getType('a') == Types.INT32

    # test 'items()'...
    h = Hash('a.b', 12, 'c.d', 23, 'e', 65, 'f.g.m', 'abrakadabra', 'x', 88.9)
    h['v'] = VectorHash([Hash('a', 21), Hash('z', Hash())])
    h['w'] = Hash('q.r', 77.7)
    # get iterator of ItemView...
    it = iter(h.items())
    k, v = next(it)
    assert k == 'a'
    assert v == Hash('b', 12)
    assert isinstance(v, Hash)
    # iterate over this Hash...
    jt = iter(v.items())
    k1, v1 = next(jt)
    assert k1 == 'b'
    assert isinstance(v1, int)
    assert v1 == 12
    with pytest.raises(StopIteration):
        next(jt)
    k, v = next(it)
    assert k == 'c'
    assert v == Hash('d', 23)
    k, v = next(it)
    assert k == 'e'
    assert v == 65
    k, v = next(it)
    assert k == 'f'
    assert v == Hash('g.m', 'abrakadabra')
    k, v = next(it)
    assert k == 'x'
    assert v == 88.9
    k, v = next(it)
    assert k == 'v'
    assert isinstance(v, VectorHash)
    assert v == VectorHash([Hash('a', 21), Hash('z', Hash())])
    assert len(v) == 2
    # iterate over VectorHash...
    jt = iter(v)
    hsh = next(jt)
    assert isinstance(hsh, Hash)
    assert hsh == Hash('a', 21)
    hsh = next(jt)
    assert hsh == Hash('z', Hash())
    with pytest.raises(StopIteration):
        next(jt)
    k, v = next(it)
    assert k == 'w'
    assert v == Hash('q.r', 77.7)
    with pytest.raises(StopIteration):
        next(it)

    # testing middlelayer-like API
    h = Hash('a.b.c', 21, 'f.g', "some string", 'i', 31.4865)
    # set attributes
    h['a.b.c', ...] = {'a': 12, 'b': {'e': "Invalid argument"}}
    assert h['a.b.c'] == 21
    assert h['i'] == 31.4865
    # retrieve attributes. Attributes syntax is 'dict'-like
    # 'attrs' is a ref to Hash node's attributes
    attrs = h['a.b.c', ...]
    assert attrs['a'] == 12
    assert attrs['b']['e'] == "Invalid argument"
    assert h['a.b.c', ...]['b']['e'] == "Invalid argument"
    assert h['a.b.c', 'b']['e'] == "Invalid argument"
    # update attributes
    h['a.b.c', ...].update({'w': True, 'b': {'e': "Valid argument"}})
    h['a.b.c', 'a'] = 13
    assert attrs['a'] == 13
    assert attrs['b']['e'] == "Valid argument"
    assert attrs['w'] is True
    h['a.b.c', 'w'] = False
    assert attrs['w'] is False
    # The 'attr' is a reference
    attrs['param'] = 'changed'
    assert h['a.b.c', ...]['param'] == 'changed'
    # ... or ...
    assert h['a.b.c', 'param'] == 'changed'
    # store attribits as a string
    sattrs = str(attrs)
    # delete parent Hash
    del h
    # check that 'attrs' reference is still alive
    try:
        s = str(attrs)
    except BaseException:
        s = None
        del attrs
    assert s == sattrs

    # add new attribute via looping with 'iterall'
    h = Hash('a', 1, 'b', 2, 'c', 3, 'd', 4)
    h['a', ...] = {'a': 12}
    h['b', ...] = {'b': 22}
    h['c', ...] = {'c': 32}
    h['d', ...] = {'d': 42}
    for k, _, a in h.iterall():
        a['w'] = 3.1415956
    assert h['a', 'a'] == 12
    assert h['a', 'w'] == 3.1415956
    assert h['b', 'b'] == 22
    assert h['b', 'w'] == 3.1415956
    assert h['c', 'c'] == 32
    assert h['c', 'w'] == 3.1415956
    assert h['d', 'd'] == 42
    assert h['d', 'w'] == 3.1415956

    # test 'HashAttributes'
    h = Hash('a.b.c', 21, 'f.g', "some string", 'i', 31.4865)
    # assign Hash attributes (lhs) the py dict (rhs) ...
    h['a.b.c', ...] = {'a': 12, 'b': {'e': "Invalid argument"}}

    g = Hash('x.y.z', 99)
    # assign attributes (python dict)
    g['x.y.z', ...] = {'x': 55, 'b': {'w': True}}
    # define the attributes ref (not a copy!)
    gattrs = g['x.y.z', ...]
    gattrs['x'] = 77
    assert g['x.y.z', 'x'] == 77
    assert g['x.y.z', ...]['x'] == 77
    assert gattrs['x'] == 77
    assert gattrs['b']['w'] is True

    # "merge"
    g['x.y.z', ...].update(h['a.b.c', ...])

    assert g['x.y.z', 'x'] == 77
    assert gattrs['x'] == 77
    assert gattrs['b']['e'] == "Invalid argument"
    assert 'w' not in gattrs['b']
    assert gattrs['a'] == 12

    # "replace" : 'g' attributes are the same as in 'h'
    # assign attributes (HashAttributes -> C++ Hash::Attributes)
    g['x.y.z', ...] = h['a.b.c', ...]

    # gattrs still points to the same object as g['x.y.z', ...]
    assert 'x' not in gattrs
    assert 'x' not in g['x.y.z', ...]
    assert gattrs['a'] == 12
    assert gattrs['b']['e'] == "Invalid argument"

    del gattrs
    del g


def test_getsetVectorHash():
    vh = VectorHash()
    vh.append(Hash('b', 1))
    vh.append(Hash('b', 2))
    h = Hash('a', vh)
    h['a'].extend((Hash('c', 3), Hash('c', 4),))
    h['a'].append(Hash('d', 5))
    assert len(h['a']) == 5
    assert h['a'][2]['c'] == 3
    assert h['a[2].c'] == 3
    assert isinstance(h, Hash)
    assert not isinstance(h['a'], Hash)
    assert isinstance(h['a'], VectorHash)
    assert isinstance(h['a'][2], Hash)
    assert isinstance(h['a[2].c'], int)
    h['a'][1]['b'] = 222
    assert h['a[1].b'] == 222

    h2 = h  # h2 points to the same tree
    h2['a[3].c'] = 33
    assert h2['a[3].c'] == 33
    assert h['a[3].c'] == 33

    h3 = copy.copy(h)  # full copy
    h3['a[3].c'] = 77
    assert h3['a[3].c'] == 77
    assert h['a[3].c'] == 33
    assert h == h2
    # assert not h == h3)

    # Check implicitly conversion [hash, hash,...] to VectorHash
    h = Hash('a', [Hash('a', 1), Hash('b', 2), Hash('c', 3)])
    assert h['a[0].a'] == 1
    assert h['a[1].b'] == 2
    assert h['a[2].c'] == 3


def test_copy_and_VectorHash():
    arr = np.arange(20000, dtype=np.int16).reshape(100, 200)
    assert arr[0][0] == 0
    v = VectorHash([Hash('a', arr), Hash('b', 2)])

    # Shallow copy ...
    vc = copy.copy(v)
    assert not id(vc) == id(v)

    vd = copy.deepcopy(v)
    assert not id(vd) == id(v)
    # Change origin vector...
    arr[0][0] = 111
    assert v[0]['a'][0][0] == 111
    # Shallow copy has seen the change ...
    assert vc[0]['a'][0][0] == 111
    # But deep copy is unchanged ...
    assert vd[0]['a'][0][0] == 0


def test_getAs():
    # BOOL
    h = Hash("a", True)
    assert h.getType("a") == Types.BOOL
    # The following tests are not needed since all conversion methods:
    # "getValueAs", "getAs", "getAttributeAs" use the same internal
    # conversion function.

    # Test "getAs"
    assert h.getAs("a", Types.BOOL)
    assert h.getAs("a", Types.CHAR) == '1'
    assert h.getAs("a", Types.INT8) == 1
    assert h.getAs("a", Types.UINT8) == 1
    assert h.getAs("a", Types.INT16) == 1
    assert h.getAs("a", Types.UINT16) == 1
    assert h.getAs("a", Types.INT32) == 1
    assert h.getAs("a", Types.UINT32) == 1
    assert h.getAs("a", Types.INT64) == 1
    assert h.getAs("a", Types.UINT64) == 1
    assert h.getAs("a", Types.FLOAT) == 1.0
    assert h.getAs("a", Types.DOUBLE) == 1.0
    assert h.getAs("a", Types.COMPLEX_FLOAT) == (1 + 0j)
    assert h.getAs("a", Types.COMPLEX_DOUBLE) == (1 + 0j)

    assert h.getAs("a", Types.STRING) == "1"
    assert h.getAs("a", Types.VECTOR_BOOL) == [True]
    # Gives bytearray(b'\xd4'):
    # assert h.getAs("a", Types.VECTOR_CHAR) == bytearray(b'\x01')
    assert h.getAs("a", Types.VECTOR_INT8) == [1]
    assert h.getAs("a", Types.VECTOR_UINT8) == [1]
    assert h.getAs("a", Types.VECTOR_INT16) == [1]
    assert h.getAs("a", Types.VECTOR_UINT16) == [1]
    assert h.getAs("a", Types.VECTOR_INT32) == [1]
    assert h.getAs("a", Types.VECTOR_UINT32) == [1]
    assert h.getAs("a", Types.VECTOR_INT64) == [1]
    assert h.getAs("a", Types.VECTOR_UINT64) == [1]
    assert h.getAs("a", Types.VECTOR_FLOAT) == [1.0]
    assert h.getAs("a", Types.VECTOR_DOUBLE) == [1.0]
    assert h.getAs("a", Types.VECTOR_COMPLEX_FLOAT) == [(1 + 0j)]
    assert h.getAs("a", Types.VECTOR_COMPLEX_DOUBLE) == [(1 + 0j)]
    assert h.getAs("a", Types.VECTOR_STRING) == ['1']


def test_getAttributeAs():
    # Test "getAttributesAs"
    h = Hash("a", True)
    h.setAttribute("a", "a", True)
    assert h.getAttributeAs("a", "a", Types.STRING), "1"
    assert h.getAttributeAs("a", "a", Types.CHAR) == '1'
    assert h.getAttributeAs("a", "a", Types.INT32) == 1
    assert h.getAttributeAs("a", "a", Types.DOUBLE) == 1.0
    assert h.getAttributeAs("a", "a", Types.VECTOR_BOOL) == [True]
    # Gives bytearray(b'\xd4')
    # assert
    #    h.getAttributeAs("a", "a", Types.VECTOR_CHAR) == bytearray(b'\x01')
    assert h.getAttributeAs("a", "a", Types.VECTOR_INT8) == [1]
    assert h.getAttributeAs("a", "a", Types.VECTOR_UINT8) == [1]
    assert h.getAttributeAs("a", "a", Types.VECTOR_INT32) == [1]
    assert h.getAttributeAs("a", "a", Types.VECTOR_DOUBLE) == [1.0]
    assert h.getAttributeAs("a", "a", Types.VECTOR_COMPLEX_FLOAT) == [(1 + 0j)]
    assert h.getAttributeAs("a", "a", Types.VECTOR_COMPLEX_DOUBLE) == [
        (1 + 0j)]
    assert h.getAttributeAs("a", "a", Types.VECTOR_STRING), ['1']

    # DOUBLE
    h.setAttribute("a", "c", 1.23)
    assert h.getAttributes("a").getNode('c').getType() == Types.DOUBLE
    assert h.getAttributeType('a', 'c') == Types.DOUBLE

    h = Hash('a', 1)
    # Check that `getAttributes(...)` return reference internal
    attrs = h.getAttributes("a")
    assert len(attrs) == 0
    h.setAttribute('a', 'a', True)
    assert len(attrs) == 1

    # Check that attributes can be copied to another Hash tree
    g = Hash("Z.a.b.c", "value")
    g.setAttributes("Z.a.b.c", attrs)
    assert g.getAttributeAs("Z.a.b.c", "a", Types.STRING) == "1"
    assert g.getAttributeAs("Z.a.b.c", "a", Types.INT32) == 1
    assert g.getAttributeAs("Z.a.b.c", "a", Types.DOUBLE) == 1.0


def test_setAs():
    # Test "setAs"
    testTypes = (Types.INT8, Types.UINT8, Types.INT16, Types.UINT16,
                 Types.INT32, Types.UINT32, Types.INT64, Types.UINT64,
                 Types.FLOAT, Types.DOUBLE)

    h = Hash()

    for t in testTypes:
        h.setAs("a", 5, t)
        assert h.getType("a") == t, "Setting as type " + str(t) + " failed"
        assert h["a"], 5 == "Equality failed for type " + str(t)

    # Special values for BOOL
    h.setAs("a", 1, "BOOL")
    assert h.getType("a") == Types.BOOL
    assert h["a"] is True

    h.setAs("a", 1.0, "VECTOR_BOOL")
    assert h.getType("a") == Types.VECTOR_BOOL
    assert h["a"] == [True]

    # Other vectors ...

    # Small positive numbers ... < 128
    h.setAs("a", [127, 0, 77], Types.VECTOR_INT8)
    assert h.getType("a") == Types.VECTOR_INT8
    # byte array: 0x7f -> 127, 0x00 -> 0, 'M' -> 77
    assert h["a"] == [127, 0, 77]

    h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_INT16)
    assert h.getType("a") == Types.VECTOR_INT16
    assert h["a"] == [1260, -21170, 0, 1]

    h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_DOUBLE)
    assert h.getType("a") == Types.VECTOR_DOUBLE
    assert h["a"] == [1260.0, -21170.0, 0.0, 1.0]

    h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_COMPLEX_FLOAT)
    assert h.getType("a") == Types.VECTOR_COMPLEX_FLOAT
    assert h["a"] == [(1260 + 0j), (-21170 + 0j), 0j, (1 + 0j)]

    h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_STRING)
    assert h.getType("a") == Types.VECTOR_STRING
    assert h["a"] == ['1260', '-21170', '0', '1']


def test_merge():
    h1 = Hash("a", 1,
              "b", 2,
              "c.b[0].g", 3,
              "c.c[0].d", 4,
              "c.c[1]", Hash("a.b.c", 6),
              "d.e", 7)
    h1.set("f.g", 99)
    h1.set("h", -1)

    # Test "setAttribute"
    h1.setAttribute("a", "attrKey", "Just a number")
    h1.setAttribute("c.b", "attrKey2", 3)
    h1.setAttribute("c.b[0].g", "attrKey3", 4.)
    h1.setAttribute("f", "attrKey6", "buaah!")

    h1b = Hash(h1)
    h1c = Hash(h1)
    h1d = Hash(h1)

    h2 = Hash("a", 21,
              "b.c", 22,
              "c.b[0]", Hash("key", "value"),
              "c.b[1].d", 24,
              "e", 27,
              "f", Hash())
    h2.set("g.h.i", -88)
    h2.set("g.h.j", -188)
    h2.set("h.i", -199)
    h2.set("h.j", 200)
    h2.set("i[3]", Hash())
    h2.set("i[1].j", 200)
    h2.set("i[2]", Hash("k.l", 5.))
    h2.set("j", Hash("k", 5.))
    h2.setAttribute("a", "attrKey", "Really just a number")
    h2.setAttribute("e", "attrKey4", -1)
    h2.setAttribute("e", "attrKey5", -11.)
    h2.setAttribute("f", "attrKey7", 77)
    # attribute on new vector<Hash> node
    h2.setAttribute("i", "attrKey8", 123)
    h2.setAttribute("j", "attrKey9", 12.3)  # ... and new Hash node

    h1.merge(h2)  # HashMergePolicy.REPLACE_ATTRIBUTES is the default
    h1b.merge(h2, HashMergePolicy.MERGE_ATTRIBUTES)
    # same as h1d.merge(h2): only check similarity and once
    # attribute replacement below
    h1d += h2

    assert similar(h1, h1b)
    assert similar(h1, h1d)

    assert h1.has("a")
    assert h1.get("a") == 21  # new value
    #  Attribute kept, but value overwritten:
    assert h1.hasAttribute("a", "attrKey"), "Attribute on node not kept"
    assert h1.getAttribute("a", "attrKey") == "Really just a number"
    assert 1 == h1.getAttributes("a").size()

    assert h1b.hasAttribute("a", "attrKey")
    assert h1b.getAttribute("a", "attrKey") == "Really just a number"

    assert 1, h1b.getAttributes("a").size()

    assert h1.has("b")
    assert h1.isType("b", Types.HASH)  # switch to new type
    assert h1.has("b.c")  # ...and as Hash can hold a child

    # Attributes overwritten by nothing or kept
    assert h1.getAttributes("c.b").size() == 0

    assert h1b.getAttributes("c.b").size() == 1
    assert h1b.hasAttribute("c.b", "attrKey2")
    assert h1b.getAttribute("c.b", "attrKey2") == 3

    assert not h1.has("c.b.d")
    assert h1.has("c.b[0]")
    assert h1.has("c.b[1]")
    assert h1.get("c.b[1].d") == 24
    assert h1.has("c.c[0].d")
    assert h1.has("c.c[1].a.b.c")
    assert h1.has("d.e")
    assert h1.has("e")
    assert h1.has("g.h.i")
    assert h1.has("g.h.j")
    assert h1.has("h.i")
    assert h1.has("h.j")
    assert h1.has("i[1].j")
    assert h1.has("i[2].k.l")
    assert h1.has("i[3]")
    assert h1.has("j.k")

    # Just add attributes with leaf (identical for REPLACE or MERGE)
    assert h1.getAttributes("e").size() == 2
    assert h1.getAttribute("e", "attrKey4") == -1
    assert h1.getAttribute("e", "attrKey5") == -11.
    assert h1b.getAttributes("e").size() == 2
    assert h1b.getAttribute("e", "attrKey4") == -1
    assert h1b.getAttribute("e", "attrKey5") == -11.
    # Just add attributes for new Hash/vector<Hash>
    # (identical for REPLACE or MERGE)
    assert h1.getAttributes("i").size() == 1
    assert h1.getAttribute("i", "attrKey8") == 123
    assert h1.getAttributes("j").size() == 1
    assert h1.getAttribute("j", "attrKey9") == 12.3

    assert h1b.getAttributes("i").size() == 1
    assert h1b.getAttribute("i", "attrKey8") == 123
    assert h1b.getAttributes("j").size() == 1
    assert h1b.getAttribute("j", "attrKey9") == 12.3
    assert h1b.hasAttribute("c.b", "attrKey2")

    assert h1.has("f")
    assert h1.has("f.g")  # merging does not overwrite h1["f"] with empty Hash

    assert h1.getAttributes("f").size() == 1
    assert h1.getAttribute("f", "attrKey7") == 77
    # += is merge with REPLACE_ATTRIBUTES
    assert h1d.getAttributes("f").size() == 1
    assert h1d.getAttribute("f", "attrKey7") == 77
    # here is MERGE_ATTRIBUTES
    assert h1b.getAttributes("f").size() == 2
    assert h1b.getAttribute("f", "attrKey6") == "buaah!"
    assert h1b.getAttribute("f", "attrKey7") == 77
    # Now check the 'selectedPaths' feature (no extra test
    # for attribute merging needed):
    selectedPaths = ["a", "b.c", "g.h.i", "h.i", "i[2]", "i[5]"]
    # check that we tolerate to select path with invalid index

    h1c.merge(h2, HashMergePolicy.MERGE_ATTRIBUTES, selectedPaths)

    # Keep everything it had before merging:
    assert h1c.has("a")
    assert h1c.has("b")
    assert h1c.has("c.b[0].g")
    assert h1c.has("c.c[0].d")
    assert h1c.has("c.c[1].a.b.c")
    assert h1c.has("d.e")
    assert h1c.has("f.g")
    # The additionally selected ones from h2:
    assert h1c.has("b.c")
    assert h1c.has("g.h.i")
    assert h1c.has("h.i")
    assert h1c.has("i[0].k.l")  # only row 2 selected, which becomes row 0
    # But not the other ones from h2:
    assert not h1c.has("c.b[0].key")  # neither at old position of h2
    assert not h1c.has("c.b[2]")  # nor an extended vector<Hash> at all
    assert not h1c.has("e")
    # Take care that adding path "g.h.i" does not trigger
    # that other children of "g.h"
    # in h2 are taken as well:
    assert not h1c.has("g.h.j")
    assert not h1c.has("h.j")
    # Adding i[2] should not trigger to add children of i[1] nor i[3]]
    assert not h1c.has("i[1].j")
    assert not h1c.has("i[3]")

    # Some further small tests for so far untested cases with
    # selected paths...
    hashTarget = Hash("a.b", 1, "a.c", Hash(), "c", "so so!")
    hashSource = Hash("a.d", 8., "ha", 9)
    selectedPaths = ("a",)
    # trigger merging a.d - tuple should work as well
    hashTarget.merge(hashSource, HashMergePolicy.MERGE_ATTRIBUTES,
                     selectedPaths)
    assert hashTarget.has("a.d")

    hashTargetB = Hash("a[1].b", 1, "c", "Does not matter")
    hashTargetC = Hash(hashTargetB)
    hashTargetD = Hash(hashTargetB)
    hashSourceBCD = Hash("a[2]", Hash("a", 33, "c", 4.4), "ha", 9,
                         "c[1]", Hash("k", 5, "l", 6),
                         "c[2]", Hash("b", -3), "d[2].b", 66,
                         "e[1]", Hash("1", 1, "2", 2, "3", 3))
    selectedPaths = ["a",  # trigger merging full vector
                     # trigger selecting first HashVec item
                     # overwriting what was not a hashVec before, but
                     # only keep selected items
                     "c[1].l",  # for rows no key selection, => '.l' is ignored
                     "d",  # trigger adding full new vector
                     "e[1].2",  # here '.2' is ignored
                     ]
    hashTargetB.merge(hashSourceBCD, HashMergePolicy.MERGE_ATTRIBUTES,
                      selectedPaths)
    assert hashTargetB.has("a[0]")  # The empty one merged into it
    assert not hashTargetB.has("a[0].b")
    assert hashTargetB.has("a[1]")  # dito
    assert not hashTargetB.has("a[1].b")  # target table a got replaced
    assert hashTargetB.has("a[2].a")
    assert not hashTargetB.has("a[5]")
    assert hashTargetB.has("c[0]")
    assert hashTargetB.has("c[0].k")
    assert hashTargetB.has("c[0].l")
    assert hashTargetB.has("d[2].b")
    assert not hashTargetB.has("d[3]")
    assert hashTargetB.has("e[0].1")
    assert hashTargetB.has("e[0].2")
    assert hashTargetB.has("e[0].3")

    selectedPaths = ("a[0]",
                     "a[2].b",  #
                     "c")  # trigger overwriting with complete vector
    hashTargetC.merge(hashSourceBCD, HashMergePolicy.MERGE_ATTRIBUTES,
                      selectedPaths)
    assert not hashTargetC.has("a[1].b")  # all table rows are overwritten
    assert hashTargetC.has("a[1].a")
    assert hashTargetC.has("a[1].c")
    assert not hashTargetC.has("a[2]")
    assert hashTargetC.has("c[1].k")
    assert hashTargetC.has("c[1].l")
    assert hashTargetC.has("c[2].b")
    assert not hashTargetC.has("c[3]")

    # Now select only invalid indices - nothing should be added
    selectedPaths = ["a[10]",  # to existing vector
                     "c[10]",  # where there was another node
                     "d[10]",  # where there was no node at all
                     "ha[0]"]  # for leaves, all indices are invalid
    copyD = Hash(hashTargetD)
    # also test use of keyword parameter:
    hashTargetD.merge(hashSourceBCD, selectedPaths=selectedPaths)
    assert similar(copyD, hashTargetD)

    h = Hash('a[0].a.b.c', 1, 'a[1].a.b.d', 2)
    g = Hash('a[0].x.y.w', 77, 'a[0].a.c', 33, 'a[2].abc', 12)
    h += g
    assert not h.has('a[0].a.b.c')  # overwritten
    assert h['a[0].x.y.w'] == 77
    assert h['a[0].a.c'] == 33
    assert not h.has('a[1].a.b.d')  # dito
    assert h['a[2].abc'] == 12


def test_subtract():
    h1 = Hash("a", 1,
              "b", 2,
              "c.b[0].g", 3,
              "c.c[0].d", 4,
              "c.c[1]", Hash("a.b.c", 6),
              "d.e", 7
              )

    h2 = Hash("a", 21,
              "b.c", 22,
              "c.b[0]", Hash("key", "value"),
              "c.b[1].d", 24,
              "e", 27
              )

    # Test "__iadd__"
    h1 += h2

    # Test "__isub__"
    h1 -= h2

    assert "a" not in h1
    assert h1["b"].empty()
    assert not h1.has("c.b[0].g")
    assert h1["c.c[0].d"] == 4
    assert h1["c.c[1].a.b.c"] == 6
    assert h1["d.e"] == 7

    h3 = Hash("a.b.c", 1,
              "a.b.d", 2,
              "b.c.d", 22,
              "c.a.b", 77,
              "c.c", "blabla"
              )
    h4 = Hash("a.b", Hash(), "c", Hash())
    h3 -= h4

    assert "a.b" in h3
    assert not h3["a"].empty()
    assert "c" in h3
    assert h3["b.c.d"] == 22


def test_erase():
    h = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3,
             'd.e', 4, 'e', 5, 'f.g.h.i.j.k', 6)
    assert 'a' in h
    assert 'f' in h
    assert 'b' in h
    assert 'c' in h
    assert 'd' in h

    paths = h.getPaths()
    for path in paths:
        if len(path) == 11:
            h.erasePath(path)
    assert 'a' not in h
    assert 'f' not in h
    assert 'b' in h
    assert 'c' in h
    assert 'd' in h

    paths = h.getPaths()
    for path in paths:
        if len(path) == 8:
            h.erasePath(path)
    assert 'a' not in h
    assert 'f' not in h
    assert 'b' not in h
    assert 'c' not in h
    assert len(h) == 2
    assert h['d.e'] == 4
    assert h['e'] == 5

    # now testing erase
    h1 = Hash('a', 1, 'b', 2, 'c.d', 31, 'e.f.g', 411, 'e.f.h', 412,
              'e.i', 42)

    assert len(h1) == 4

    # erase existing key on first level => size decreases
    assert h1.erase('a')
    assert 'a' not in h1
    assert len(h1) == 3

    # non-existing key - return false and keep size:
    assert not h1.erase('a')
    assert len(h1) == 3

    # 'c.d': composite key without siblings
    assert h1.erase('c.d')
    assert 'c.d' not in h1
    assert 'c' in h1
    assert len(h1) == 3  # 'c' still in!

    # 'e.f': composite key with two children and a sibling
    assert h1.erase('e.f')
    assert 'e.f.g' not in h1
    assert 'e.f.h' not in h1
    assert 'e.f' not in h1
    assert 'e' in h1  # stays
    assert len(h1) == 3


def test_fullEqual():
    h = Hash('a', 1, 'b', 2.2, 'c.d', "text",
             'e.f.g', VectorHash())
    h.setAttribute('a', "attr", 42)

    # A copy fully equals:
    h1 = copy.copy(h)
    assert fullyEqual(h1, h), "h1: " + str(h1)
    assert fullyEqual(h1, h, False), "h1: " + str(h1)

    # A changed value leads to being different
    h2 = copy.copy(h)
    h2["c.d"] = "TEXT"
    assert not fullyEqual(h2, h), "h2: " + str(h2)
    assert not fullyEqual(h2, h, False), "h2: " + str(h2)

    # An added value leads to being different
    h3 = copy.copy(h)
    h3["newKey"] = -11
    assert not fullyEqual(h3, h), "h3: " + str(h3)
    assert not fullyEqual(h3, h, False), "h3: " + str(h3)

    # A changed attribute leads to being different
    h4 = copy.copy(h)
    h4.setAttribute("a", "attr", -42)
    assert not fullyEqual(h4, h), "h4: " + str(h4)
    assert not fullyEqual(h4, h, False), "h4: " + str(h4)

    # An added attribute leads to being different
    h5 = copy.copy(h)
    h5.setAttribute("a", "newAttr", 0)
    assert not fullyEqual(h5, h), "h5: " + str(h5)
    assert not fullyEqual(h5, h, False), "h5: " + str(h5)

    # A changed order of keys leads to being different,
    # except if flag to ignore order is given
    h6 = copy.copy(h)
    bValue = h6["b"]
    h6.erase("b")
    h6["b"] = bValue
    assert not fullyEqual(h6, h), "h6: " + str(h6)
    assert fullyEqual(h6, h, False), "h6: " + str(h6)

    # A changed order of attribute keys leads to being different,
    # except if flag to ignore order is given
    h1.setAttribute("a", "attr2", 43)  # 2nd attribute
    h7 = copy.copy(h1)
    attr2Value = h7.getAttribute("a", "attr")
    h7.getAttributes("a").erase("attr")  # erase 1st attribute
    # re-add after previous 2nd
    h7.setAttribute("a", "attr", attr2Value)
    assert not fullyEqual(h7, h1), "h7: " + str(h7)
    assert fullyEqual(h7, h1, False), "h7: " + str(h7)


def test_ndarray():
    arr = np.array([[1, 2, 3, 4, 5], [6, 7, 8, 9, 10],
                    [11, 12, 13, 14, 15]])
    # The array owner is Python
    assert arr.flags.owndata
    # Put array into Hash ...
    h = Hash('a', arr)
    # The array owner inside the Hash is not a C++ code ...
    assert not h['a'].flags.owndata
    # The other attributes are not changed ...
    assert h['a'].dtype == np.dtype('int64')
    assert h['a'][0][0] == 1
    assert h['a'][0][3] == 4
    assert h['a'][2][1] == 12
    assert h['a'].size == 15
    assert h['a'].itemsize == 8
    assert h['a'].nbytes == 120
    assert h['a'].ndim == 2
    assert h['a'].shape == (3, 5)
    assert h['a'].strides == (40, 8)
    assert h['a'].flags.c_contiguous
    assert h['a'].flags.carray
    assert h['a'].flags.aligned
    assert h['a'].flags.writeable
    assert isinstance(h['a'], np.ndarray)

    # copy.copy will NOT copy the underlying C++ NDArray
    h2 = copy.copy(h)

    assert isinstance(h2['a'], np.ndarray)
    assert h2['a'][0][0] == 1

    h['a'][0][0] = 255

    assert h['a'][0][0] == 255
    assert h2['a'][0][0] == 255
    assert arr[0][0] == 255

    # Check 'deepcopy': it copies even underlying NDArray
    g = copy.deepcopy(h)
    assert g['a'][0][0] == 255

    h['a'][0][0] = 1

    assert h['a'][0][0] == 1
    assert g['a'][0][0] == 255

    # Check fortran order array:
    # Since the C++ NDArray only knows C order, it has to be copied
    arr = np.array([[1, 2, 3], [4, 5, 6]], order='F')
    assert arr.flags.f_contiguous
    assert not arr.flags.c_contiguous

    h['b'] = arr  # copies!
    h_arr = h['b']
    assert not h_arr.flags.f_contiguous
    assert h_arr.flags.c_contiguous
    assert h_arr[0][0] == 1
    assert h_arr[0][1] == 2
    assert h_arr[0][2] == 3
    assert h_arr[1][0] == 4
    assert h_arr[1][1] == 5
    assert h_arr[1][2] == 6
    arr[0][0] == 11
    assert h_arr[0][0] == 1  # untouched

    arr = np.arange(20000, dtype=np.int16).reshape(100, 200)
    arr.flags.writeable = False
    h = Hash()
    h['a'] = arr
    assert np.all(h['a'] == arr) is np.True_
    # Can't insert non-writable arrays into Hash
    # with pytest.raises(ValueError):
    #     h['a'] = arr
