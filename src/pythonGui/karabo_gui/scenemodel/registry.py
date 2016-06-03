from collections import defaultdict, namedtuple

from .const import NS_KARABO

_scene_readers = defaultdict(list)
_scene_writers = {}
RegistryEntry = namedtuple("RegistryEntry", ['function', 'version'])


def get_reader(version):
    """ Return a reader function for the appropriate version number.
    """
    readers = {}
    for name, entries in _scene_readers.items():
        filtered = [e for e in entries if e.version == version]
        entry = filtered[0] if len(filtered) > 0 else entries[-1]
        readers[name] = entry.function

    def _get_reader_klass(element):
        for kind in ('widget', 'class'):
            klass = element.get(NS_KARABO + kind)
            if klass is not None:
                return klass
        return element.tag

    def _reader_entrypoint(element):
        klass = _get_reader_klass(element)
        reader = readers.get(klass)
        assert reader is not None, 'Reader not found for {}!'.format(element)
        return reader(_reader_entrypoint, element)

    return _reader_entrypoint


def get_writer():
    """ Return a writer function for all registered classes.
    """
    def _writer_entrypoint(obj, element):
        entry = _scene_writers.get(obj.__class__)
        return entry.function(_writer_entrypoint, obj, element)
    return _writer_entrypoint


class register_scene_reader(object):
    """ Decorator for reader functions.
    """
    def __init__(self, objname, xmltag='', version=1):
        self.objname = objname
        self.xmltag = xmltag
        self.version = version

    def __call__(self, func):
        entry = RegistryEntry(func, version=self.version)
        self.__sorted_insert(self.objname, entry)
        if self.xmltag:
            self.__sorted_insert(self.xmltag, entry)

        return func

    @staticmethod
    def __sorted_insert(klass, entry):
        global _scene_readers

        readers = _scene_readers[klass]
        readers.append(entry)
        readers.sort(key=lambda f: f.version)


class register_scene_writer(object):
    """ Decorator for writer functions
    """
    def __init__(self, objclass):
        self.objclass = objclass

    def __call__(self, func):
        global _scene_writers
        _scene_writers[self.objclass] = RegistryEntry(func, 0)
        return func
