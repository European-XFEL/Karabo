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
import numbers
from collections import OrderedDict
from collections.abc import Iterable
from copy import deepcopy
from enum import Enum

import numpy as np
import tabulate

from .typenums import XML_TYPE_TO_HASH_TYPE, HashType

__all__ = ['get_hash_type_from_data', 'Hash', 'HashByte', 'HashElement',
           'HashList', 'HashMergePolicy', 'is_equal', 'Schema',
           'simple_deepcopy']


class HashElement:
    __slots__ = ["data", "attrs"]

    def __init__(self, data, attrs):
        self.data = data
        self.attrs = attrs

    def __eq__(self, other):
        if isinstance(other, HashElement):
            def _equal(d0, d1):
                ret = (d0 == d1)
                return all(ret) if isinstance(ret, Iterable) else ret

            return (_equal(self.data, other.data) and
                    _equal(self.attrs, other.attrs))
        return super().__eq__(other)

    def __iter__(self):
        """Iter implementation for tuple based unpacking"""
        yield self.data
        yield self.attrs


SEPARATOR = "."


class Hash(OrderedDict):
    """This is the serialization data structure of Karabo

    Every data that gets transferred over the network or saved to file
    by Karabo is in this format.

    It is mostly an extended :class:`dict`.

    The big difference to normal Python containers is the dot-access method.
    The hash has a built-in knowledge about it containing itself. Thus,
    one can access sub-hashes by ``hash['key.subhash']``.

    The other speciality are attributes. In Python, these can be accessed
    using a second parameter to the brackets, as in
    ``hash['key', 'attribute']``.

    All attributes at the same time can be accessed by ``hash['key', ...]``."""

    _hashType = HashType.Hash

    def __init__(self, *args):
        if len(args) == 1:
            if isinstance(args[0], Hash):
                OrderedDict.__init__(self)
                for k, v, a in args[0].iterall():
                    OrderedDict.__setitem__(self, k, HashElement(v, a))
            else:
                OrderedDict.__init__(self, args[0])
        else:
            OrderedDict.__init__(self)
            for k, v in zip(args[::2], args[1::2]):
                self.setElement(k, v, {})

    def _path(self, path, auto=False):
        path = path.split(SEPARATOR)
        s = self
        for p in path[:-1]:
            if auto and p not in s:
                OrderedDict.__setitem__(s, p, HashElement(Hash(), {}))
            s = OrderedDict.__getitem__(s, p).data
        if not isinstance(s, Hash):
            raise KeyError(path)
        return s, path[-1]

    def _get(self, path, auto=False):
        if SEPARATOR not in path:
            # We can use the fast path here for methods like ``__contains__``
            return OrderedDict.__getitem__(self, path)

        return OrderedDict.__getitem__(*self._path(path, auto))

    def __str__(self):
        r = ', '.join(f'{k}{self[k, ...]!r}: {self[k]!r}'
                      for k in self)
        return '<' + r + '>'

    def __repr__(self):
        """Return the printable representation of the `Hash`

        the representation will contain types.
        please note that the types are not accurate if the
        data cannot be serialised."""
        if self.empty():
            return "<>"
        tabular = "  "

        def _is_table(value):
            """Check if the value belong to a table value"""
            if (isinstance(value, list) and len(value)
                    and isinstance(value[0], Hash)):
                return HashList.hashlist_format(value) is HashListFormat.Table
            return False

        def _pretty_generator(h, n=0):
            for key, value, attrs in h.iterall():
                if isinstance(value, Hash):
                    yield tabular * n + f"{key}{attrs!r}\n"
                    yield from _pretty_generator(value, n + 2)
                elif _is_table(value):
                    yield tabular * n + f"{key}{attrs!r}\n"
                    for row in value.__repr__().split("\n"):
                        yield tabular * n + row + "\n"
                else:
                    try:
                        hash_type = get_hash_type_from_data(value).name
                    except (ValueError, TypeError):
                        hash_type = "Unknown"
                    yield (tabular * n + f"{key}{attrs!r}: "
                                         f"{value!r} => {hash_type}\n")

        return f"<\n{''.join(_pretty_generator(self))}>"

    def _setelement(self, key, value):
        # NOTE: This is a fast path for __setitem__ to be use by the binary
        # deserializer. It must only be called for values in this hash, never
        # for values in sub-hashes!
        assert '.' not in key, "Can't set values in sub-hashes!"
        OrderedDict.__setitem__(self, key, value)

    def getElement(self, path):
        """This is a direct way of getting `value` and `attrs` of the `Hash`
        in a `HashElement` object.

        :returns: A tuple of value and attributes belonging to `path`, e.g.
                  data, attrs = hash.getElement(key)
        """
        path = str(path)
        if SEPARATOR not in path:
            element = OrderedDict.__getitem__(self, path)
        else:
            element = self._get(path)

        return element.data, element.attrs

    def setElement(self, key, value, attrs):
        """This is a direct way of setting `value` and `attrs` in the `Hash`

        Setting both `value` and `attrs` at the same time can provide a fairly
        big speedup! The attributes `attrs` have to be a dictionary.
        """
        assert isinstance(attrs, dict)
        element = HashElement(value, attrs)
        # Note: The karabo Hash only handles string keys...
        key = str(key)
        if SEPARATOR not in key:
            OrderedDict.__setitem__(self, key, element)
        else:
            sub_hash, path = self._path(key, True)
            OrderedDict.__setitem__(sub_hash, path, element)

    __marker = object()

    def pop(self, item, default=__marker):
        """Pop an item from the Hash. This method only returns the value."""
        if item in self:
            ret = self[item]
            del self[item]
            return ret
        if default is self.__marker:
            raise KeyError(item)
        return default

    def __setitem__(self, item, value):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                self._get(key).attrs = value
            else:
                self._get(key).attrs[attr] = value
        else:
            item = str(item)
            if SEPARATOR not in item:
                if item not in self:
                    OrderedDict.__setitem__(self, item, HashElement(value, {}))
                else:
                    attrs = OrderedDict.__getitem__(self, item).attrs
                    OrderedDict.__setitem__(self, item,
                                            HashElement(value, attrs))
            else:
                s, p = self._path(item, True)
                if p in s:
                    attrs = OrderedDict.__getitem__(s, p).attrs
                else:
                    attrs = {}
                OrderedDict.__setitem__(s, p, HashElement(value, attrs))

    def __getitem__(self, item):
        """Get an item from the Hash. Specify either a single `key` or both
        key and attribute with [`key`, `attribute`] to retrieve data.

        To retrieve the full attribute dictionary for a specific key, use
        the `Ellipsis` with [`key`, ...]"""
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                return self._get(key).attrs
            else:
                return self._get(key).attrs[attr]
        else:
            if SEPARATOR not in item:
                return OrderedDict.__getitem__(self, item).data
            return self._get(item).data

    def __delitem__(self, item):
        """Delete an `item` from the Hash"""
        if isinstance(item, tuple):
            key, attr = item
            del self._get(key).attrs[attr]
        else:
            OrderedDict.__delitem__(*self._path(item))

    def __contains__(self, key):
        """Retrieve if a key is contained in the Hash

        :returns: True or False
        """
        try:
            self._get(key)
            return True
        except KeyError:
            return False

    def iterall(self):
        """ Iterate over key, value and attributes

        This behaves like the :meth:`~dict.items` method of Python
        :class:`dict`, except that it yields not only key and value but
        also the attributes for it.
        """
        # NOTE: Because this only iterates over a single level of the Hash,
        # none of the keys contain '.' and thus OrderedDict.__getitem__ can
        # be called directly for a fairly big speedup
        for k in self:
            elem = OrderedDict.__getitem__(self, k)
            yield k, elem.data, elem.attrs

    def items(self):
        """Iterate over key and values of the Hash container"""
        for k in self:
            yield k, OrderedDict.__getitem__(self, k).data

    def merge(self, other, attribute_policy='merge'):
        """Merge the hash other into this hash.

        If the *attribute_policy* is ``'merge'``, the attributes from the other
        hash are merged with the existing ones, otherwise they are overwritten.
        """
        merge = attribute_policy == "merge"
        for k, v in other.items():
            if isinstance(v, Hash):
                if k not in self or self[k] is None:
                    self[k] = Hash()
                self[k].merge(v, attribute_policy)
            else:
                self[k] = v
            if merge:
                self[k, ...].update(other[k, ...])
            else:
                self[k, ...] = other[k, ...].copy()

    def get(self, item, default=None):
        """The graceful item getter function of the Hash

        :param item: The item key for the element
        :param default: The return value if the item is not available
        """
        try:
            return self[item]
        except KeyError:
            return default

    def set(self, item, value):
        self[item] = value

    def setAttribute(self, item, key, value):
        self[item, key] = value

    def getAttribute(self, item, key):
        return self[item, key]

    def getAttributes(self, item):
        """Retrieve all attributes related to `item`"""
        return self[item, ...]

    def has(self, item):
        """Check if the `item` is contained in the Hash"""
        return item in self

    def getKeys(self, keys=None):
        """
        Retrieve a list of keys from the Hash. If the 'keys' parameter
        is provided, the specified keys are added to the given list.

        :param  keys :A list of keys to be added to the result.
        Example:
            h = Hash('a', 1, 'b', 2)
            assert h.getKeys() == ['a', 'b']
            custom_keys = ['c', 'd']
            assert h.getKeys(keys=custom_keys) == ['c', 'd', 'a', 'b']
        """
        if keys is None:
            return list(self.keys())
        return keys.extend(list(self.keys()))

    def hasAttribute(self, item, key):
        """Check if the attribute associated with `item` and `key` is
        contained the Hash"""
        return key in self[item, ...]

    def erase(self, key):
        """Erase an item associated with `key` from the Hash"""
        del self[key]

    def paths(self, *, intermediate=True):
        """Returns all root-to-leaves paths

        :param intermediate: If `True` include all intermediate path from
                             roots to leafs, i.e. ['a.b.c', 'a.b', 'a']
                             instead of ['a.b.c']
        """

        def full_paths(hsh):
            ret = []
            for k, v in hsh.items():
                if isinstance(v, Hash):
                    ret.extend(k + '.' + kk for kk in full_paths(v))
                ret.append(k)
            return ret

        def leaf_paths(hsh, keys=[]):
            ret = []
            for k, v in hsh.items():
                if isinstance(v, Hash) and v:
                    ret.extend(leaf_paths(v, keys=keys + [k]))
                else:
                    ret.append('.'.join(keys + [k]))
            return ret

        return full_paths(self) if intermediate else leaf_paths(self)

    def empty(self):
        """Check if the hash is empty or not

        :returns: True or False
        """
        return len(self) == 0

    def __deepcopy__(self, memo):
        Cls = type(self)
        ret = Cls.__new__(Cls)
        # let deepcopy know now about us!
        memo[id(self)] = ret
        # Take a list for protection from threaded access
        iterable = list(Hash.flat_iterall(self, empty=True))
        for key, value, attrs in iterable:
            ret.setElement(key, deepcopy(value, memo), deepcopy(attrs, memo))

        return ret

    def deepcopy(self):
        """This method retrieves a quick deepcopy of the `Hash` element

        This method bypasses `copy.deepcopy`, assuming we only copy
        'simple' datastructures.
        """
        ret = Hash()
        # Take a list for protection from threaded access
        iterable = list(Hash.flat_iterall(self, empty=True))
        for key, value, attrs in iterable:
            ret[key] = simple_deepcopy(value)
            copy_attr = {}
            for ak, av in attrs.items():
                copy_attr[ak] = simple_deepcopy(av)
            ret[key, ...] = copy_attr

        return ret

    def fullyEqual(self, other):
        """Compare two `Hash` objects and check if they have equal content

        Note: This function does not consider the insertion order of elements
        """
        assert isinstance(other, Hash)

        # Do the fast path check first!
        h_paths = sorted(self.paths(intermediate=True))
        other_paths = sorted(other.paths(intermediate=True))
        if h_paths != other_paths:
            return False

        # Take a list for protection from threaded access
        iterable = list(Hash.flat_iterall(other, empty=True))
        for key, value, attr in iterable:
            # We only have to compare values if we do not have a Hash. This
            # is accounted in the paths. But we must compare attributes!
            if not isinstance(value, Hash):
                h_value = self[key]
                if not is_equal(value, h_value):
                    return False

            h_attr = self[key, ...]
            # We can check the attributes `keys` first!
            h_attr_keys = sorted(h_attr.keys())
            a_keys = sorted(attr.keys())
            if h_attr_keys != a_keys:
                return False
            for a_key, a_value in attr.items():
                if not is_equal(a_value, h_attr[a_key]):
                    return False

        return True

    @staticmethod
    def flat_iterall(hsh, base='', empty=False):
        """Recursively iterate over all parameters in a Hash object such that
        a simple iterator interface is exposed.

        :param empty: If set to `True` this will yield an empty Hash followed
                      by all Hash properties. Default is `False`
        """
        assert isinstance(hsh, Hash)

        base = base + '.' if base else ''
        for key, value, attrs in hsh.iterall():
            subkey = base + key
            if isinstance(value, Hash):
                if empty:
                    # Yield an empty Hash as base value
                    yield subkey, Hash(), attrs
                yield from Hash.flat_iterall(
                    value, base=subkey, empty=empty)
            else:
                yield subkey, value, attrs


class HashMergePolicy:
    MERGE_ATTRIBUTES = "merge"
    REPLACE_ATTRIBUTES = "replace"


class HashListFormat:
    Unknown = 0
    Table = 1
    ListOfNodes = 2


class HashList(list):
    _hashType = HashType.VectorHash

    @staticmethod
    def hashlist_format(hash_list) -> HashListFormat:
        """Check the format of a HashList

        The known formats in Karabo that use a `HashList` are::

          - Tables: A Table has a header and each Hash contains a value for
                    each header key

          - ListOfNodes: Must have a single key for each Hash and the value
                         is a Hash
        """
        if not hash_list:
            return HashListFormat.Unknown

        hsh = hash_list[0]
        if hsh.empty():
            # Hash is empty that must never happen!
            return HashListFormat.Unknown

        def equal(iterator):
            iterator = iter(iterator)
            try:
                first = next(iterator)
            except StopIteration:
                return True
            return all(first == x for x in iterator)

        equal_sizes = equal([len(hsh.keys()) for hsh in hash_list])
        if not equal_sizes:
            return HashListFormat.Unknown

        value = hsh[next(iter(hsh))]
        hash_value = isinstance(value, Hash)
        from_lon = hash_value and len(hsh) == 1
        if from_lon:
            return HashListFormat.ListOfNodes

        equal_keys = equal([hsh.keys() for hsh in hash_list])
        from_table = not hash_value and equal_keys
        if from_table:
            return HashListFormat.Table

        return HashListFormat.Unknown

    def _safe_repr(self):
        """A safe repr wrapper to convert ndarray (vectors) to lists"""
        return [{k: v.tolist() if isinstance(v, np.ndarray) else v
                 for k, v in row.items()} for row in self]

    def __repr__(self):
        """Return the pretty representation of a HashList"""
        if not len(self):
            return "<HashList([])>"

        fmt = self.hashlist_format(self)
        if fmt is HashListFormat.Table:
            return tabulate.tabulate(
                self._safe_repr(), headers="keys", tablefmt="grid")
        else:
            # XXX: More support for other HashLists
            return "<HashList(" + super().__repr__() + ")>"


class HashByte(str):
    """This represents just one byte, so that we can distinguish
    CHAR and VECTOR_CHAR."""
    _hashType = HashType.Char

    def __repr__(self):
        return f"${ord(self):x}"


class Schema:
    """This represents a Karabo Schema, it encapsulates a schema `hash`
    and has a `name`.
    """
    _hashType = HashType.Schema

    def __init__(self, name=None, *, hash=None):
        self.name = name
        if hash is None:
            self.hash = Hash()
        else:
            self.hash = hash

    def copy(self, other):
        self.hash = Hash()
        self.hash.merge(other.hash)
        self.name = other.name

    def keyHasAlias(self, key):
        return "alias" in self.hash[key, ...]

    def getAliasAsString(self, key):
        if self.hash.hasAttribute(key, "alias"):
            return self.hash[key, "alias"]

    def getKeyFromAlias(self, alias):
        for k in self.hash.paths(intermediate=True):
            if alias == self.hash[k, ...].get("alias", None):
                return k

    def __eq__(self, other):
        if (other.__class__ is self.__class__
                and other.name == self.name and other.hash == self.hash):
            return True
        return False

    def __hash__(self):
        return id(self)

    def getValueType(self, key):
        return XML_TYPE_TO_HASH_TYPE[self.hash[key, "valueType"]]

    def filterByTags(self, *args):
        args = set(args)
        h = Hash()
        for k in self.hash.paths(intermediate=True):
            tags = self.hash[k, ...].get("tags", ())
            if not args.isdisjoint(tags):
                h[k] = self.hash[k]
        return h

    def __repr__(self):
        return f"Schema('{self.name}', {self.hash})"


NUMPY_TO_HASH_TYPE_VECTOR = {
    np.bool_: HashType.VectorBool,
    np.int8: HashType.VectorInt8,
    np.uint8: HashType.VectorUInt8,
    np.int16: HashType.VectorInt16,
    np.uint16: HashType.VectorUInt16,
    np.int32: HashType.VectorInt32,
    np.uint32: HashType.VectorUInt32,
    np.int64: HashType.VectorInt64,
    np.uint64: HashType.VectorUInt64,
    np.float32: HashType.VectorFloat,
    np.float64: HashType.VectorDouble,
    np.complex64: HashType.VectorComplexFloat,
    np.complex128: HashType.VectorComplexDouble,
}

NUMPY_TO_HASH_TYPE_SIMPLE = {
    np.bool_: HashType.Bool,
    np.int8: HashType.Int8,
    np.uint8: HashType.UInt8,
    np.int16: HashType.Int16,
    np.uint16: HashType.UInt16,
    np.int32: HashType.Int32,
    np.uint32: HashType.UInt32,
    np.int64: HashType.Int64,
    np.uint64: HashType.UInt64,
    np.float32: HashType.Float,
    np.float64: HashType.Double,
    np.complex64: HashType.ComplexFloat,
    np.complex128: HashType.ComplexDouble,
}


def get_hash_type_from_data(data):
    """Get a `HashType` enum corresponding to `data`

    :param data: Any kind of data typically used in the control network
    :returns: HashType Enum
    """
    return HashType(_get_hash_num_from_data(data))


def _get_hash_type_by_class(data):
    if hasattr(data, '_hashType'):
        return data._hashType
    elif isinstance(data, bool):
        return HashType.Bool
    elif isinstance(data, (Enum, numbers.Integral)):
        return HashType.Int32
    elif isinstance(data, numbers.Real):
        return HashType.Double
    elif isinstance(data, numbers.Complex):
        return HashType.ComplexDouble
    elif isinstance(data, bytes):
        return HashType.VectorChar
    elif isinstance(data, str):
        return HashType.String
    elif data is None:
        return HashType.None_


types_cache = {}


def _get_hash_num_from_data(data):
    """internal speed optimized version of `get_hash_type_from_data`

    it returns the type number in a hash instead of the `HashType`,
    and, especially, caches results where possible.
    """

    if (ret := types_cache.get(type(data))) is not None:
        return ret

    try:
        if data.ndim == 1 and isinstance(data, np.ndarray):
            return NUMPY_TO_HASH_TYPE_VECTOR[data.dtype.type].value
        else:
            ret = NUMPY_TO_HASH_TYPE_SIMPLE[data.dtype.type]
            types_cache[type(data)] = ret.value
            return ret.value
    except AttributeError:
        if (ret := _get_hash_type_by_class(data)) is not None:
            types_cache[type(data)] = ret.value
            return ret.value
        elif isinstance(data, list):
            if data:
                return _get_hash_num_from_data(data[0]) + 1
            else:
                return HashType.VectorString.value
        try:
            memoryview(data)
            return HashType.ByteArray.value
        except TypeError:
            raise TypeError(f'unknown datatype {data.__class__}')


def is_equal(a, b):
    """A compare function deals with element-wise comparison"""
    try:
        res = (a == b)
    except ValueError:
        # If shapes can't be broadcasted together, we have a dimension
        # mismatch of ndarrays
        array_check = map(lambda x: isinstance(x, np.ndarray), (a, b))
        assert any(array_check)
        return False

    return all(res) if isinstance(res, Iterable) else res


def simple_deepcopy(value):
    """A simple and quick deepcopy mechanism for simple data structures
    """
    try:
        # dicts, sets, ndarrays
        v = value.copy()
    except TypeError:
        # Must be schema
        assert isinstance(value, Schema)
        cpy = Schema()
        cpy.copy(value)
        v = cpy
    except AttributeError:
        try:
            # lists, tuples, strings, unicode
            v = value[:]
        except TypeError:
            # Simple values
            v = value
    return v
