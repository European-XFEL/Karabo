import importlib
import os

import pkg_resources
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
    name = binding.displayed_name
    return "{} [{}]".format(name, unit) if unit else name


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
            importlib.import_module('{}.{}'.format(pkg, submodule))

    load_extensions()


def load_extensions():
    """Load the entrypoints for our gui extensions"""
    EXT_PACKAGE = 'karabogui.gui_extensions'

    entry_points = pkg_resources.iter_entry_points(EXT_PACKAGE)
    for entry in entry_points:
        try:
            entry.load()
        except Exception as e:
            print('Cannot load plugin {}: {}'.format(entry.name, e))


def with_display_type(display_type):
    """Create a checker function for the `is_compatible` argument of
    `register_binding_controller` which looks for a specific display type
    of property.
    """

    def is_compatible(binding):
        dt = binding.display_type
        return dt == display_type

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
                     binding.required_access_level)
    return is_accessible and is_allowed
