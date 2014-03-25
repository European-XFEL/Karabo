#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on March 19, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


import hashtypes

from collections import OrderedDict
from xml.etree import ElementTree
from cStringIO import StringIO
import numbers
import numpy

from struct import pack, unpack, calcsize

def schemaloader(s):
    name, xml = s.split(":", 1)
    ret = Schema()
    ret.hash = parseXML(xml)
    ret.name = name
    return ret

def _gettype(data):
    try:
        if isinstance(data, numpy.ndarray):
            return hashtypes.NumpyVector.vstrs[data.dtype.str]
        else:
            return hashtypes.Type.strs[data.dtype.str]
    except AttributeError:
        if isinstance(data, bool):
            return hashtypes.Bool
        elif isinstance(data, numbers.Integral):
            return hashtypes.Int64
        elif isinstance(data, numbers.Real):
            return hashtypes.Double
        elif isinstance(data, numbers.Complex):
            return hashtypes.Complex
        elif isinstance(data, bytes):
            return hashtypes.VectorChar
        elif isinstance(data, unicode):
            return hashtypes.String
        elif isinstance(data, Hash):
            return hashtypes.Hash
        elif isinstance(data, list):
            return _gettype(data[0]).vectortype
        elif data is None:
            return hashtypes.None_
        else:
            raise RuntimeError('unknown datatype {}'.format(data.__class__))


class Element(object):
    text = property(lambda self: None, lambda self, value: None)
    tail = text

    def __init__(self, tag, attrs={}):
        self.tag = tag
        if "KRB_Artificial" in attrs:
            self.artificial = True
        self.type = attrs.get("KRB_Type")
        def parse(vv):
            k, v = vv.split(":", 1)
            return hashtypes.Type.fromname[k[4:]].fromstring(v)
        self.attrs = {k: parse(v) for k, v in attrs.iteritems()
                      if k[:4] != "KRB_"}


    def items(self):
        yield "KRB_Type", self.hashname()
        for k, v in self.attrs.iteritems():
            t = _gettype(v)
            yield k, u'KRB_{}:{}'.format(t.hashname(), v)


class SimpleElement(Element):
    def __len__(self):
        return 0

    def append(self, elem):
        raise RuntimeError("no append to simple element")

    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self

    def __iter__(self):
        return
        yield

    @property
    def text(self):
        try:
            return _gettype(self.data).toString(self.data)
        except AttributeError:
            return None

    @text.setter
    def text(self, value):
        try:
            self.data = hashtypes.Type.fromname[self.type].fromstring(value)
        except:
            raise


    def hashname(self):
        return _gettype(self.data).hashname()


class HashElement(Element):
    def __init__(self, tag, attrs={}):
        Element.__init__(self, tag, attrs)
        self.children = Hash()

    def __len__(self):
        return len(self.children)

    def append(self, elem):
        OrderedDict.__setitem__(self.children, elem.tag, elem)

    def __iter__(self):
        for e in self.children:
            yield OrderedDict.__getitem__(self.children, e)

    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self
        for e in self.children:
            for e in OrderedDict.__getitem__(self.children, e).iter(tag):
                yield e

    @property
    def data(self):
        return self.children


    def hashname(self):
        return "HASH"


class ListElement(Element):
    def __init__(self, tag, attrs={}):
        Element.__init__(self, tag, attrs)
        self.children = [ ]


    def __len__(self):
        return len(self.children)


    def append(self, elem):
        self.children.append(elem)


    def __iter__(self):
        return iter(self.children)


    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self
        for e in self.children:
            for ee in e.iter(tag):
                yield ee


    @property
    def data(self):
        return [e.data for e in self.children]


    @data.setter
    def data(self, value):
        self.children = [ ]
        for c in value:
            e = HashElement('KRB_Item')
            e.children = c
            self.children.append(e)


    def hashname(self):
        return "VECTOR_HASH"


class Hash(OrderedDict):
    """This is a replacement for the Karabo C++ Hash

    It has most of the C++ functionality, plus some typical
    python methods.

    The bit difference to normal python containers is the dot-access method.
    The hash has a built-in knowledge about it containing itself. Thus,
    one can access subhashes by hash['key.subhash'].

    The other speciality are attributes. In python, these can be accessed
    using a second parameter to the brackets, as in hash['key', 'attribute'].
    All attributes at the same time can be accessed by hash['key', ...]."""
    def __init__(self, *args):
        if len(args) == 1:
            OrderedDict.__init__(self, args[0])
        else:
            OrderedDict.__init__(self)
            for k, v in zip(args[::2], args[1::2]):
                self[k] = v

    def _path(self, path, auto=False):
        path = path.split(".")
        s = self
        for p in path[:-1]:
            if auto and p not in s:
                OrderedDict.__setitem__(s, p, HashElement(p))
            s = s[p]
        return s, path[-1]
    
    def _get(self, path, auto=False):
        return OrderedDict.__getitem__(*self._path(path, auto))


    def __str__(self):
        r = ', '.join('{}{}: {}'.format(k, self[k, ...], self[k])
                      for k in self)
        return '<' + r + '>'


    def __setitem__(self, item, value):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                self._get(key).attrs = value
            else:
                self._get(key).attrs[attr] = value
        else:
            s, p = self._path(unicode(item), True)
            if p in s:
                attrs = s[p, ...]
            else:
                attrs = { }
            if isinstance(value, Hash):
                elem = HashElement(p)
                elem.children = value
            elif isinstance(value, list):
                elem = ListElement(p)
                elem.data = value
            else:
                elem = SimpleElement(p)
                elem.data = value
            elem.attrs = attrs
            OrderedDict.__setitem__(s, p, elem)

    def __getitem__(self, item):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                return self._get(key).attrs
            else:
                return self._get(key).attrs[attr]
        else:
            try:
                return self._get(item).data
            except AttributeError:
                return None

    def __delitem__(self, item):
        if isinstance(item, tuple):
            key, attr = item
            del self._get(key).attrib[attr]
        else:
            OrderedDict.__delitem__(*self._path(item))


    def __contains__(self, key):
        try:
            self._get(key)
            return True
        except KeyError:
            return False


    def iterall(self):
        for k in self:
            yield k, self[k], self[k, ...]


    def merge(self, other, attribute_policy):
        """Merge the hash other into this hash.

        If the attribute_policy is 'merge', the attributes from the other
        hash are merged with the existing ones, otherwise they are overwritten.
        """
        merge = attribute_policy == "merge"
        for k, v in other.iteritems():
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


    def get(self, item):
        return self[item]

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
            return self.keys()
        return keys.extend(self.keys())

    def hasAttribute(self, item, key):
        return key in self[item, ...]

    def erase(self, key):
        del self[key]

    def paths(self):
        ret = [ ]
        for k, v in self.iteritems():
            if isinstance(v, Hash):
                ret.extend([k + '.' + kk for kk in v.paths()])
            else:
                ret.append(k)
        return ret

    def empty(self):
        return len(self) == 0

class HashMergePolicy:
    MERGE_ATTRIBUTES = "merge"
    REPLACE_ATTRIBUTES = "replace"

class Schema(object):
    def getKeys(self):
        return self.hash.keys()

def factory(tag, attrs):
    if attrs["KRB_Type"] == "HASH":
        return HashElement(tag, attrs)
    elif attrs["KRB_Type"] == "VECTOR_HASH":
        return ListElement(tag, attrs)
    else:
        return SimpleElement(tag, attrs)


class XMLParser(object):
    def read(self, data):
        """Parse the XML in the buffer data and return the hash"""
        target = ElementTree.TreeBuilder(element_factory=factory)
        parser = ElementTree.XMLParser(target=target)
        parser.feed(data)
        root = target.close()
        if hasattr(root, "artificial"):
            return root.children
        else:
            ret = Hash()
            OrderedDict.__setitem__(ret, root.tag, root)
            return ret


class Writer(object):
    def write(self, data):
        """Return the written data as a string"""
        self.file = StringIO()
        try:
            self.writeToFile(data, self.file)
            return self.file.getvalue()
        finally:
            self.file.close()


class XMLWriter(Writer):
    def writeToFile(self, hash, file):
        """Write the hash to the file in binary format"""
        if len(hash) == 1 and isinstance(hash.values()[0], Hash):
            e = OrderedDict.__getitem__(hash, hash.keys()[0])
        else:
            e = HashElement("root")
            e.attrs = dict(KRB_Artificial="", KRB_Type="HASH")
            e.children = hash
        et = ElementTree.ElementTree(e)
        et.write(file)


class BinaryParser(object):
    def readFormat(self, fmt):
        size = calcsize(fmt)
        self.pos += size
        return unpack(fmt, self.data[self.pos - size:self.pos])


    def readKey(self):
        size, = self.readFormat('B')
        self.pos += size
        return self.data[self.pos - size:self.pos]


    def read(self, data):
        self.pos = 0
        self.data = data
        return hashtypes.Hash.read(self)


class BinaryWriter(Writer):
    def writeFormat(self, fmt, data):
        s = pack(fmt, data)
        self.file.write(s)


    def writeKey(self, key):
        key = key.encode('utf8')
        self.writeFormat('B', len(key))
        self.file.write(key)


    def writeType(self, data):
        type = _gettype(data)
        if type == hashtypes.VectorChar:
            print 'writing vc', data
        self.writeFormat('I', type.number)


    def writeData(self, data):
        type = _gettype(data)
        type.write(self, data)


    def writeToFile(self, data, file):
        self.file = file
        hashtypes.Hash.write(self, data)


class Schema(object):
    def __init__(self, name, hash):
        self.name = name
        self.hash = hash
