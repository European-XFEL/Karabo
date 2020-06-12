from collections import namedtuple
from inspect import signature

from traits.api import (
    cached_property, Callable, Dict, HasStrictTraits, Instance, Int, Property,
    String)

from .const import NS_KARABO, SCENE_FILE_VERSION, UNKNOWN_WIDGET_CLASS

_scene_writers = {}
RegistryEntry = namedtuple("RegistryEntry", ['function', 'version'])


class ReaderEntry(HasStrictTraits):

    functions = Dict(Int, Callable)  # `{version: function}`
    latest_function = Property(depends_on="functions")
    single_function = Property(depends_on="functions")

    @cached_property
    def _get_latest_function(self):
        latest = max(sorted(self.functions.keys()))
        return self.functions[latest]

    @cached_property
    def _get_single_function(self):
        if len(self.functions) == 1:
            return list(self.functions.values())[0]
        return None

    def get_function(self, version):
        """Usually there is only one function for a reader. If it's not the
           case, we look up with the version. If no version exists, we default
           to the latest."""
        return (self.single_function
                or self.functions.get(version)
                or self.latest_function)


class ReaderRegistry(HasStrictTraits):
    """This is a singleton"""

    entries = Dict(String, Instance(ReaderEntry))  # `{name: entry}`
    version = Int
    defs = Dict

    def register(self, name=None, func=None, version=None):
        entry = self.entries.get(name)
        if entry is None:
            entry = ReaderEntry()
            self.entries[name] = entry
        entry.functions[version] = func

    def read(self, element):
        klass = self._fetch_klass(element)
        reader = self._fetch_reader(name=klass, version=self.version)
        assert reader is not None, 'Reader not found for {}!'.format(element)

        sig = signature(reader)
        # XXX: Backward compatibility with GUI extensions. Old readers have
        # two (2) arguments, new ones have a single argument
        if len(sig.parameters) == 2:
            return reader(None, element)

        return reader(element)

    def add_defs(self, models):
        self.defs.clear()
        for d in models:
            for child in d.children:
                self.defs[child.id] = child

    def find(self, id_):
        return self.defs.get(id_)

    def _fetch_klass(self, element):
        for kind in ('widget', 'class'):
            klass = element.get(NS_KARABO + kind)
            if klass is not None:
                if klass in self.entries:
                    return klass
                else:
                    # Allow for the default widget reader
                    return UNKNOWN_WIDGET_CLASS
        if element.tag in self.entries:
            return element.tag
        # Allow for a default reader
        return '*'

    def _fetch_reader(self, name, version):
        return self.entries[name].get_function(version)


# The reader registry contains the functions registered as readers.
# This shouldn't be used outside of this module.
# Please use the following convenience functions
#    1. set_reader_registry_function
#    2. read_element
_reader_registry = ReaderRegistry()


def set_reader_registry_version(version=SCENE_FILE_VERSION):
    """Set the version to the global scene reader registry. This is usually
    done on the start of scene reading."""
    _reader_registry.version = version


def read_element(element):
    """Read an XML element with the entries from the global scene reader
    registry. """
    global _reader_registry
    return _reader_registry.read(element)


def add_temporary_defs(defs):
    """For every scene opening, the SVG defs are temporarily added in the
       registry. These can be then used by the readers with `find_def`.

       This is very useful for SVG elements with defs such as a
       line with a marker def (arrow)"""
    global _reader_registry
    _reader_registry.add_defs(defs)


def find_def(id_):
    global _reader_registry
    return _reader_registry.find(id_)


def get_writer():
    """ Return a writer function for all registered classes.
    """
    global _scene_writers

    def _writer_entrypoint(obj, element):
        entry = _scene_writers.get(obj.__class__)
        return entry.function(_writer_entrypoint, obj, element)
    return _writer_entrypoint


class register_scene_reader(object):
    """ Decorator for reader functions.
    """
    def __init__(self, objname, xmltag='', version=SCENE_FILE_VERSION):
        self.objname = objname
        self.xmltag = xmltag
        self.version = version

    def __call__(self, func):
        global _reader_registry
        _reader_registry.register(name=self.objname, func=func,
                                  version=self.version)
        if self.xmltag:
            _reader_registry.register(name=self.xmltag, func=func,
                                      version=self.version)

        return func


class register_scene_writer(object):
    """ Decorator for writer functions
    """
    def __init__(self, objclass):
        self.objclass = objclass

    def __call__(self, func):
        global _scene_writers
        _scene_writers[self.objclass] = RegistryEntry(func, 0)
        return func
