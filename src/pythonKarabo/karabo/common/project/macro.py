#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import List, Property, String, cached_property

from .bases import BaseProjectObjectModel


class MacroModel(BaseProjectObjectModel):
    """ An object representing the data for a Karabo Python macro.
    """
    # The instance ID of the running macro
    instance_id = Property(String, depends_on=['simple_name', 'uuid'])
    # The instance names of all active macros
    instances = List(String, transient=True)
    # The actual macro source
    code = String()

    @cached_property
    def _get_instance_id(self):
        return "Macro-{}-{}".format(self.simple_name, self.uuid)


def read_macro(filename_or_fileobj):
    """ Read a macro and return it.
        ``filename_or_fileobj`` is either a string containing a filename, or a
        file-like object which can be read from (eg- a StringIO instance).
        If ``filename_or_fileobj`` is None, an empty MacroModel is returned.
    """
    if filename_or_fileobj is None:
        return MacroModel()

    if not hasattr(filename_or_fileobj, 'read'):
        with open(filename_or_fileobj, 'r') as input:
            macro_code = input.read()
    else:
        macro_code = filename_or_fileobj.read()

    model = MacroModel(code=macro_code)
    model.initialized = True  # Do this last to avoid triggering `modified`
    return model


def write_macro(macro):
    """ Write MacroModel object `macro` to a string.
    """
    return macro.code
