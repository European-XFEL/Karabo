#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import cached_property, HasStrictTraits, List, Property, String


def read_macro(filename_or_fileobj):
    """ Read a macro and return it.
        filename_or_fileobj is either a string containing a filename, or a
        file-like object which can be read from (eg- a StringIO instance).
    """
    if not hasattr(filename_or_fileobj, 'read'):
        with open(filename_or_fileobj, 'r') as input:
            macro_code = input.read()
    else:
        macro_code = filename_or_fileobj.read()
    return MacroModel(code=macro_code)


def write_macro(macro):
    """ Write MacroModel object `macro` to a string.
    """
    return macro.code


class MacroModel(HasStrictTraits):
    """ An object representing the data for a Karabo GUI macro."""
    # The title of the macro
    title = String()
    # The instance ID of the running macro
    instance_id = Property(String, depends_on=['title'])
    # The instance names of all active macros
    instances = List(String)
    # The actual macro source
    code = String()

    @cached_property
    def _get_instance_id(self):
        return "Macro-{}".format(self.title)
