from karabogui.binding.api import (
    KARABO_SCHEMA_DISPLAYED_NAME, KARABO_SCHEMA_DISPLAY_TYPE)


def axis_label(proxy):
    """Return the axis label for a PropertyProxy instance
    """
    binding = proxy.binding
    if binding is None:
        return ''

    unit = binding.unit_label
    name = binding.attributes.get(KARABO_SCHEMA_DISPLAYED_NAME, '')
    return "{} [{}]".format(name, unit) if unit else name


def has_options(binding):
    """Returns True if a binding has any `options` defined."""
    return len(binding.options) > 0


def with_display_type(display_type):
    """Create a checker function for the `is_compatible` argument of
    `register_binding_controller` which looks for a specific display type
    of property.
    """
    def is_compatible(binding):
        dt = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE)
        return dt == display_type

    return is_compatible
