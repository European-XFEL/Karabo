from .hash import Hash
from .typenums import XML_TYPE_TO_HASH_TYPE, HashType

__all__ = ['Schema']


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
