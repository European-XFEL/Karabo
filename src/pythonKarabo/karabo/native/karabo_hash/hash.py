from collections import OrderedDict, Iterable
from copy import deepcopy
from enum import Enum
import numbers

import numpy as np

from .typenums import HashType, XML_TYPE_TO_HASH_TYPE

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


SEPARATOR = "."


class Hash(OrderedDict):
    """This is the serialization data structure of Karabo

    Every data that gets transfered over the network or saved to file
    by Karabo is in this format.

    It is mostly an extended :class:`dict`.

    The big difference to normal Python containers is the dot-access method.
    The hash has a built-in knowledge about it containing itself. Thus,
    one can access subhashes by ``hash['key.subhash']``.

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
                self[k] = v

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

    def __repr__(self):
        r = ', '.join('{}{!r}: {!r}'.format(k, self[k, ...], self[k])
                      for k in self)
        return '<' + r + '>'

    def _setelement(self, key, value):
        # NOTE: This is a fast path for __setitem__ to be use by the binary
        # deserializer. It must only be called for values in this hash, never
        # for values in sub-hashes!
        assert '.' not in key, "Can't set values in sub-hashes!"
        OrderedDict.__setitem__(self, key, value)

    def setElement(self, key, value, attrs):
        """This is a direct way of setting `value` and `attrs` in the `Hash`

        Setting both `value  and `attrs` at the same time can provide a fairly
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
        if isinstance(item, tuple):
            key, attr = item
            del self._get(key).attrs[attr]
        else:
            OrderedDict.__delitem__(*self._path(item))

    def __contains__(self, key):
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
        return self[item, ...]

    def has(self, item):
        return item in self

    def getKeys(self, keys=None):
        if keys is None:
            return list(self.keys())
        return keys.extend(list(self.keys()))

    def hasAttribute(self, item, key):
        return key in self[item, ...]

    def erase(self, key):
        del self[key]

    def paths(self):
        ret = []
        for k, v in self.items():
            if isinstance(v, Hash):
                ret.extend(k + '.' + kk for kk in v.paths())
            ret.append(k)
        return ret

    def empty(self):
        return len(self) == 0

    def __deepcopy__(self, memo):
        Cls = type(self)
        ret = Cls.__new__(Cls)
        # let deepcopy know now about us!
        memo[id(self)] = ret
        for key, value, attrs in Hash.flat_iterall(self, empty=True):
            ret[key] = deepcopy(value, memo)
            ret[key, ...] = deepcopy(attrs, memo)

        return ret

    def deepcopy(self):
        """This method retrieves a quick deepcopy of the `Hash` element

        This method bypasses `copy.deepcopy`, assuming we only copy
        'simple' datastructures.
        """
        ret = Hash()
        for key, value, attrs in Hash.flat_iterall(self, empty=True):
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
        h_paths = sorted(self.paths())
        other_paths = sorted(other.paths())
        if h_paths != other_paths:
            return False

        for key, value, attr in Hash.flat_iterall(other, empty=True):
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

        :param empty: Sets if empty Hashes should be returned (default: False)
        """
        assert isinstance(hsh, Hash)

        base = base + '.' if base else ''
        for key, value, attrs in hsh.iterall():
            subkey = base + key
            if isinstance(value, Hash):
                if empty and value.empty():
                    yield subkey, value, attrs
                else:
                    yield from Hash.flat_iterall(
                        value, base=subkey, empty=empty)
            else:
                yield subkey, value, attrs


class HashMergePolicy:
    MERGE_ATTRIBUTES = "merge"
    REPLACE_ATTRIBUTES = "replace"


class HashList(list):
    _hashType = HashType.VectorHash

    def __repr__(self):
        return "HashList(" + super(HashList, self).__repr__() + ")"


class HashByte(str):
    """This represents just one byte, so that we can distinguish
    CHAR and VECTOR_CHAR."""
    _hashType = HashType.Char

    def __repr__(self):
        return "${:x}".format(ord(self))


class Schema:
    _hashType = HashType.Schema

    def __init__(self, name=None, rules=None, hash=None):
        self.name = name
        if hash is None:
            self.hash = Hash()
        else:
            self.hash = hash
        self.rules = rules

    def copy(self, other):
        self.hash = Hash()
        self.hash.merge(other.hash)
        self.name = other.name
        self.rules = other.rules

    def keyHasAlias(self, key):
        return "alias" in self.hash[key, ...]

    def getAliasAsString(self, key):
        if self.hash.hasAttribute(key, "alias"):
            return self.hash[key, "alias"]

    def getKeyFromAlias(self, alias):
        for k in self.hash.paths():
            if alias == self.hash[k, ...].get("alias", None):
                return k

    def getValueType(self, key):
        return XML_TYPE_TO_HASH_TYPE[self.hash[key, "valueType"]]

    def filterByTags(self, *args):
        args = set(args)
        h = Hash()
        for k in self.hash.paths():
            tags = self.hash[k, ...].get("tags", ())
            if not args.isdisjoint(tags):
                h[k] = self.hash[k]
        return h

    def __repr__(self):
        return "Schema('{}', {})".format(self.name, self.hash)


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
    try:
        if data.ndim == 1 and isinstance(data, np.ndarray):
            return NUMPY_TO_HASH_TYPE_VECTOR[data.dtype.type]
        else:
            return NUMPY_TO_HASH_TYPE_SIMPLE[data.dtype.type]
    except AttributeError:
        if hasattr(data, '_hashType'):
            return data._hashType
        elif isinstance(data, bool) or hasattr(data, '_hashBool'):
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
        elif isinstance(data, list):
            if data:
                subtype = get_hash_type_from_data(data[0])
                return HashType(subtype.value + 1)
            else:
                return HashType.VectorString
        elif data is None or hasattr(data, '_hashNone'):
            return HashType.None_
        try:
            memoryview(data)
            return HashType.ByteArray
        except TypeError:
            raise TypeError('unknown datatype {}'.format(data.__class__))


def is_equal(a, b):
    """A compare function deals with element-wise comparison result and
    Schema object comparison
    """
    type_check = map(lambda x: isinstance(x, Schema), (a, b))
    if any(type_check):
        if all(type_check):
            # Compare Schema objects' names and hashes
            return a.name == b.name and a.hash == b.hash
        else:
            # one of a, b is not Schema, simply return False
            return False
    res = (a == b)
    # comparison of numpy arrays result in an array
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
