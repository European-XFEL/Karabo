from collections import OrderedDict
import numbers

import numpy as np


class Hash(OrderedDict):
    """This is the fundamental data structure of Karabo

    All data that gets transferred over the network or saved to file by Karabo
    is in this format.

    It is mostly an extended :class:`dict`.

    The bit of difference with normal python containers is the dot-access
    method. The hash has a built-in knowledge about it containing itself. Thus,
    one can access subhashes by ``hash['key.subhash']``.

    The other speciality are attributes. In python, these can be accessed using
    a second parameter to the brackets, as in ``hash['key', 'attribute']``.

    All attributes at the same time can be accessed by ``hash['key', ...]``.
    """

    def __init__(self, *args):
        self.__node_attrs__ = _get_empty_attr_dict('Hash')
        if len(args) == 1:
            arg = args[0]
            if isinstance(arg, Hash):
                super(Hash, self).__init__()
                for k, v, a in arg.iterall():
                    self[k] = v
                    self[k, ...] = dict(a)
            else:
                super(Hash, self).__init__(arg)
        else:
            super(Hash, self).__init__(self)
            for k, v in zip(args[::2], args[1::2]):
                self[k] = v

    def _path(self, path, auto=False):
        path = path.split(".")
        hsh = self
        for p in path[:-1]:
            if auto and p not in hsh:
                hsh.__setitem__(p, Hash())
            hsh = hsh[p]

        if not isinstance(hsh, Hash):
            raise KeyError(path)

        return hsh, path[-1]

    def _get(self, path, auto=False):
        node, key = self._path(path, auto)
        return node.__getitem__(key)

    def __repr__(self):
        def _attr_str(attrs):
            return ', '.join("'{}': {!r}".format(k, attrs[k])
                             for k in sorted(attrs.keys()))
        r = ', '.join('{}{{{}}}: {!r}'.format(k, _attr_str(self[k, ...]),
                                              self[k])
                      for k in self)
        return '<' + r + '>'

    def __setitem__(self, key, value):
        if isinstance(key, tuple):
            key, attrkey = key
            node, key = self._path(str(key), auto=True)
            if attrkey is Ellipsis:
                node.__node_attrs__[key] = value
            else:
                value_type = _get_type(value)
                node_attrs = node.__node_attrs__[key]
                node_attrs[attrkey] = value
                node_attrs['KRB_AttrTypes'][attrkey] = value_type
        elif '.' in key:
            node, key = self._path(str(key), auto=True)
            if key not in node:
                value_type = _get_type(value)
                node_attrs = _get_empty_attr_dict(value_type)
                node.__node_attrs__[key] = node_attrs
            node.__setitem__(key, value)
        else:
            if key not in self.__node_attrs__:
                value_type = _get_type(value)
                node_attrs = _get_empty_attr_dict(value_type)
                self.__node_attrs__[key] = node_attrs
            super(Hash, self).__setitem__(key, value)

    def __getitem__(self, key):
        if isinstance(key, tuple):
            key, attrkey = key
            node, key = self._path(str(key))
            if attrkey is Ellipsis:
                return node.__node_attrs__[key]
            else:
                return node.__node_attrs__[key][attrkey]
        elif '.' in key:
            return self._get(key)
        else:
            return super(Hash, self).__getitem__(key)

    def __delitem__(self, key):
        if isinstance(key, tuple):
            key, attrkey = key
            node, key = self._path(str(key))
            assert attrkey is not Ellipsis, "Attributes can not be bulk del'd"
            del node.__node_attrs__[key][attrkey]
        elif '.' in key:
            node, key = self._path(key)
            node.__delitem__(key)
        else:
            super(Hash, self).__delitem__(key)

    def __contains__(self, key):
        try:
            self._get(key)
            return True
        except KeyError:
            return False

    def iterall(self):
        """ Iterate over key, value and attributes

        This behaves like the ``items()`` method of python :class:`dict`,
        just that it yields not only key and value but also the attributes
        for it.
        """
        for k in self:
            yield k, self[k], self[k, ...]

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

    def paths(self):
        ret = []
        for k, v in self.items():
            if isinstance(v, Hash):
                ret.extend(k + '.' + kk for kk in v.paths())
            ret.append(k)
        return ret


class Schema(object):
    def __init__(self, name=None, rules=None, hash=None):
        self.name = name
        if hash is None:
            self.hash = Hash()
        else:
            self.hash = hash
        self.rules = rules


# Helper functions

def _get_empty_attr_dict(value_type):
    return {'KRB_Type': value_type, 'KRB_AttrTypes': {}}


def _get_type(value):
    """ Figure out the KRB_Type to mark a value with.
    """
    if isinstance(value, np.ndarray):
        numpy_type_names = {
            np.bool_: 'BoolArray',
            np.int8: 'Int8Array',
            np.uint8: 'UInt8Array',
            np.int16: 'Int16Array',
            np.uint16: 'UInt16Array',
            np.int32: 'Int32Array',
            np.uint32: 'UInt32Array',
            np.int64: 'Int64Array',
            np.uint64: 'UInt64Array',
            np.float32: 'Float32Array',
            np.float64: 'Float64Array',
            np.complex64: 'Complex64Array',
            np.complex128: 'Complex128Array',
        }
        return numpy_type_names[value.dtype.type]
    elif isinstance(value, Hash):
        return 'Hash'
    elif isinstance(value, Schema):
        return 'Schema'
    elif isinstance(value, bool):
        return 'Bool'
    elif isinstance(value, numbers.Integral):
        return 'Int32'
    elif isinstance(value, numbers.Real):
        return 'Float64'
    elif isinstance(value, numbers.Complex):
        return 'Complex128'
    elif isinstance(value, bytes):
        return 'Bytes'
    elif isinstance(value, str):
        return 'String'
    elif isinstance(value, list):
        if value:
            subtype = _get_type(value[0])
            return subtype + 'List'
        else:
            return 'StringList'
    elif value is None:
        return 'None'
    else:
        raise TypeError('unknown data type "{0}"'.format(value.__class__))
