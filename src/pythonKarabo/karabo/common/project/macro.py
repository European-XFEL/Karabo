#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import cached_property, List, Property, String

from .bases import BaseProjectObjectModel


class MacroModel(BaseProjectObjectModel):
    """ An object representing the data for a Karabo Python macro.
    """
    # The title of the macro
    title = String()  # XXX: Use `super().simple_name` in the future
    # The instance ID of the running macro
    instance_id = Property(String, depends_on=['title', 'project_name'])
    # The instance names of all active macros
    instances = List(String)
    # The actual macro source
    code = String()
    # The name of the project this macro belongs to
    project_name = String()

    @cached_property
    def _get_instance_id(self):
        return "Macro-{}-{}".format(self.project_name, self.title)


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
    return MacroModel(code=macro_code)


def write_macro(macro):
    """ Write MacroModel object `macro` to a string.
    """
    return macro.code
