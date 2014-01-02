from collections import OrderedDict
from xml.etree import ElementTree
from cStringIO import StringIO
import numpy

def schemaloader(s):
    name, xml = s.split(":", 1)
    ret = Schema()
    ret.hash = parseXML(xml)
    ret.name = name
    return ret

class Element(object):
    text = property(lambda self: None, lambda self, value: None)
    tail = text

    types = {
        "BOOL": lambda s: bool(int(s)),
        "STRING": unicode,
        "INT32": int,
        "UINT32": numpy.uint32,
        "UINT16": numpy.uint16,
        "DOUBLE": float,
        "VECTOR_STRING": lambda s: s.split(","),
        "VECTOR_INT32": lambda s: numpy.array([int(x) for x in s.split(",")]),
        "SCHEMA": schemaloader }

    def __init__(self, tag, attrs={}):
        self.tag = tag
        if "KRB_Artificial" in attrs:
            self.artificial = True
        self.type = attrs.get("KRB_Type")
        def parse(vv):
            k, v = vv.split(":", 1)
            return self.types[k[4:]](v)
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
            self.data = self.types[self.type](value)
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

    def __setitem__(self, item, value):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                self._get(key).attrs = value
            else:
                self._get(key).attrs[attr] = value
        else:
            s, p = self._path(unicode(item))
            if isinstance(value, Hash):
                elem = HashElement(p)
                elem.children = value
            else:
                elem = SimpleElement(p)
                elem.data = value
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
            if k in self and merge:
                self[k, ...].update(other[k, ...])
            if isinstance(v, Hash):
                if k not in self:
                    self[k] = Hash()
                self[k].merge(v, merge)
            else:
                self[k] = v
            if not merge:
                self[k, ...] = other[k, ...].copy()


    def get(self, item):
        return self[item]

    def set(self, item, value):
        self[item] = value

    def setAttribute(self, item, key, value):
        self[item, key] = value

    def getAttribute(self, item, key):
        return self[item, key]

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
