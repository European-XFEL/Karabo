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
from collections import namedtuple

from traits.api import (
    Any, Callable, Dict, HasStrictTraits, Instance, Int, Property, String,
    cached_property)

from .const import NS_KARABO, SCENE_FILE_VERSION, UNKNOWN_WIDGET_CLASS

_scene_writers = {}
RegistryEntry = namedtuple("RegistryEntry", ["function", "version"])


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
        return (
            self.single_function
            or self.functions.get(version)
            or self.latest_function
        )


class ReaderRegistry(HasStrictTraits):
    """This is a singleton"""

    entries = Dict(String, Instance(ReaderEntry))  # `{name: entry}`
    version = Int

    def register(self, name=None, func=None, version=None):
        entry = self.entries.get(name)
        if entry is None:
            entry = ReaderEntry()
            self.entries[name] = entry
        entry.functions[version] = func

    def read(self, element):
        klass = self._fetch_klass(element)
        reader = self._fetch_reader(name=klass, version=self.version)
        assert reader is not None, f"Reader not found for {element}!"
        return reader(element)

    def _fetch_klass(self, element):
        for kind in ("widget", "class"):
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
        return "*"

    def _fetch_reader(self, name, version):
        return self.entries[name].get_function(version)


class WriterRegistry(HasStrictTraits):

    entries = Dict(Any, Callable)

    def register(self, klass=None, func=None):
        self.entries[klass] = func

    def write(self, model, parent):
        klass = model.__class__
        writer = self.entries.get(klass)
        assert writer is not None, f"Writer not found for {klass}!"

        return writer(model, parent)


# The reader registry contains the functions registered as readers.
# This shouldn't be used outside of this module.
# Please use the following convenience functions
#    1. set_reader_registry_function
#    2. read_element
_reader_registry = ReaderRegistry()
_writer_registry = WriterRegistry()


def set_reader_registry_version(version=SCENE_FILE_VERSION):
    """Set the version to the global scene reader registry. This is usually
    done on the start of scene reading."""
    global _reader_registry
    _reader_registry.version = version


def read_element(element):
    """Read an XML element with the entries from the global scene reader
    registry."""
    global _reader_registry
    return _reader_registry.read(element)


def write_element(model, parent):
    global _writer_registry
    return _writer_registry.write(model, parent)


class register_scene_reader:
    """Decorator for reader functions."""

    def __init__(self, objname, xmltag="", version=SCENE_FILE_VERSION):
        self.objname = objname
        self.xmltag = xmltag
        self.version = version

    def __call__(self, func):
        global _reader_registry
        _reader_registry.register(
            name=self.objname, func=func, version=self.version
        )
        if self.xmltag:
            _reader_registry.register(
                name=self.xmltag, func=func, version=self.version
            )

        return func


class register_scene_writer:
    """Decorator for writer functions"""

    def __init__(self, objclass):
        self.objclass = objclass

    def __call__(self, func):
        global _writer_registry
        _writer_registry.register(klass=self.objclass, func=func)
        return func
