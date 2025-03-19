# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import importlib
import importlib.metadata
import os

from traits.api import Undefined

import karabogui.access as krb_access
from karabo.common.api import KARABO_SCHEMA_REGEX


def axis_label(proxy):
    """Return the axis label for a PropertyProxy instance
    """
    binding = proxy.binding
    if binding is None:
        return ''

    unit = binding.unit_label
    name = binding.displayedName
    return f"{name} [{unit}]" if unit else name


def get_class_const_trait(klass, name):
    """Return the value of a `Constant` trait which has been added to a class
    object by `register_binding_controller`.
    """
    trait = klass.class_traits()[name]
    return trait.default


def has_options(binding):
    """Returns True if a binding has any `options` defined."""
    return len(binding.options) > 0


def has_regex(binding):
    """Returns True if the binding has any `regex` defined."""
    return binding.attributes.get(KARABO_SCHEMA_REGEX) is not None


def get_regex(binding, default=None):
    """Returns the `regex` of a binding, if not present takes the `default`"""
    return binding.attributes.get(KARABO_SCHEMA_REGEX, default)


def populate_controller_registry():
    """Make sure all the `BaseBindingController` subclasses are registered.

    Yup. gui_registry_loader lives on! Hopefully a little less ugly in this
    form?

    NOTE: This will not work if the GUI is ever packaged as compiled modules
    (ie: if the .py files are not present).
    """
    ROOT_PACKAGE = 'karabogui.controllers'
    SUBPACKAGES = ('display', 'edit')
    for pkg in SUBPACKAGES:
        pkg = ROOT_PACKAGE + '.' + pkg
        module = importlib.import_module(pkg)
        # Only the Python files
        mod_files = [fn for fn in os.listdir(os.path.dirname(module.__file__))
                     if fn.endswith('.py')]
        for mod_fname in mod_files:
            submodule = os.path.splitext(mod_fname)[0]
            importlib.import_module(f'{pkg}.{submodule}')

    load_extensions()


def load_extensions():
    """Load the entrypoints for our gui extensions"""
    EXT_PACKAGE = 'karabogui.gui_extensions'

    all_entry_points = importlib.metadata.entry_points()
    entry_points = all_entry_points.select(group=EXT_PACKAGE)
    for entry in entry_points:
        try:
            entry.load()
        except Exception as e:
            print(f'Cannot load plugin {entry.name}: {e}')


def with_display_type(displayType):
    """Create a checker function for the `is_compatible` argument of
    `register_binding_controller` which looks for a specific display type
    of property.
    """

    def is_compatible(binding):
        dt = binding.displayType
        return dt == displayType

    return is_compatible


def is_proxy_allowed(proxy):
    """Retrieve if the reconfiguration of the ``PropertyProxy`` is allowed
    """
    root_proxy = proxy.root_proxy
    value = root_proxy.state_binding.value
    if value is Undefined or not value:
        return False

    binding = proxy.binding
    is_allowed = binding.is_allowed(value)
    is_accessible = (krb_access.GLOBAL_ACCESS_LEVEL >=
                     binding.requiredAccessLevel)
    return is_accessible and is_allowed
