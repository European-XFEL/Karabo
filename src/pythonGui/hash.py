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
        return self.attrs.items()

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
            return str(self.data)
        except AttributeError:
            return None

    @text.setter
    def text(self, value):
        try:
            self.data = hashtypes.Type.fromname[self.type].fromstring(value)
        except:
            raise

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

class Hash(OrderedDict):
    def __init__(self, *args):
        OrderedDict.__init__(self)
        for k, v in zip(args[::2], args[1::2]):
            self[k] = v

    def _path(self, path):
        path = path.split(".")
        s = self
        for p in path[:-1]:
            s = s[p]
        return s, path[-1]
    
    def _get(self, path):
        return OrderedDict.__getitem__(*self._path(path))


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
            s, p = self._path(unicode(item))
            if p in s:
                attrs = s[p, ...]
            else:
                attrs = { }
            if isinstance(value, Hash):
                elem = HashElement(p)
                elem.children = value
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
            except:
                raise

    def __delitem__(self, item):
        if isinstance(item, tuple):
            key, attr = item
            del self._get(key).attrib[attr]
        else:
            OrderedDict.__delitem__(*self._path(item))

    def merge(self, other, attribute_policy):
        merge = attribute_policy == "merge"
        for k, v in other.iteritems():
            if isinstance(v, Hash):
                if k not in self:
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
        try:
            self._get(item)
            return True
        except KeyError:
            return False

    def getKeys(self, keys):
        return keys.extend(self.keys())

    def hasAttribute(self, item, key):
        return key in self[item, ...]

    def paths(self):
        ret = [ ]
        for k, v in self.iteritems():
            if isinstance(v, Hash):
                ret.extend([k + '.' + kk for kk in v.paths()])
            else:
                ret.append(k)
        return ret

class HashMergePolicy:
    MERGE_ATTRIBUTES = "merge"
    REPLACE_ATTRIBUTES = "replace"

class Schema(object):
    def getKeys(self):
        return self.hash.keys()

def factory(tag, attrs):
    if attrs["KRB_Type"] == "HASH":
        return HashElement(tag, attrs)
    else:
        return SimpleElement(tag, attrs)

def parseXML(xml):
    target = ElementTree.TreeBuilder(element_factory=factory)
    parser = ElementTree.XMLParser(target=target)
    parser.feed(xml)
    root = target.close()
    if hasattr(root, "artificial"):
        return root.children
    else:
        ret = Hash()
        OrderedDict.__setitem__(ret, root.tag, root)
        return ret

def writeXML(hash):
    ret = StringIO()
    try:
        if len(hash) == 1 and isinstance(hash.values()[0], Hash):
            e = OrderedDict.__getitem__(hash, hash.keys()[0])
        else:
            e = HashElement("root")
            e.attrs = dict(KRB_Artificial="", KRB_Type="HASH")
            e.children = hash
        et = ElementTree.ElementTree(e)
        et.write(ret)
        rets = ret.getvalue()
        return rets
    finally:
        ret.close()


class BinaryParser(object):
    def readFormat(self, fmt):
        size = calcsize(fmt)
        self.pos += size
        return unpack(fmt, self.data[self.pos - size:self.pos])


    def readKey(self):
        size, = self.readFormat('B')
        self.pos += size
        return self.data[self.pos - size:self.pos]


    def read(self, file):
        self.pos = 0
        self.data = file
        return hashtypes.Hash.read(self)


class BinaryWriter(object):
    def writeFormat(self, fmt, data):
        s = pack(fmt, data)
        self.file.write(s)


    def writeKey(self, key):
        key = str(key)
        self.writeFormat('B', len(key))
        self.file.write(key)


    def _gettype(self, data):
        try:
            return hashtypes.Type.strs[data.dtype.str]
        except AttributeError:
            if isinstance(data, numbers.Integral):
                return hashtypes.Int64
            elif isinstance(data, numbers.Real):
                return hashtypes.Double
            elif isinstance(data, numbers.Complex):
                return hashtypes.Complex
            elif isinstance(data, (str, unicode)):
                return hashtypes.String
            elif isinstance(data, Hash):
                return hashtypes.Hash
            elif isinstance(data, list):
                return self._gettype(data[0])
            else:
                raise RuntimeError('unknown datatype {}'.format(data.__class__))


    def writeType(self, type):
        type = self._gettype(type)
        self.writeFormat('I', type.number)


    def writeData(self, data):
        type = self._gettype(data)
        type.write(self, data)


    def write(self, data):
        self.file = StringIO()
        try:
            hashtypes.Hash.write(self, data)
            return self.file.getvalue()
        finally:
            self.file.close()


class Schema(object):
    def __init__(self, name, hash):
        self.name = name
        self.hash = hash
