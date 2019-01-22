import importlib
import os


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


def with_display_type(display_type):
    """Create a checker function for the `is_compatible` argument of
    `register_binding_controller` which looks for a specific display type
    of property.
    """
    def is_compatible(binding):
        dt = binding.display_type
        return dt == display_type

    return is_compatible
