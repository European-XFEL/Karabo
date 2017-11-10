from karabo_gui.binding.api import KARABO_SCHEMA_DISPLAY_TYPE


def with_display_type(display_type):
    """Create a checker function for the `is_compatible` argument of
    `register_binding_controller` which looks for a specific display type
    of property.
    """
    def is_compatible(binding):
        dt = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE)
        return dt == display_type

    return is_compatible
