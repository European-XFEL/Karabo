from karabo.middlelayer import Bool, Char, Integer, Number, String
import karabo_gui.icons as icons


def get_icon(descriptor):
    """Get the proper icon to show next to a property in the configurator
    """
    if descriptor.options is not None:
        return icons.enum

    icon = icons.undefined
    if isinstance(descriptor, Char):
        icon = icons.string
    elif isinstance(descriptor, String):
        if descriptor.displayType in ('directory', 'fileIn', 'fileOut'):
            icon = icons.path
        else:
            icon = icons.string
    elif isinstance(descriptor, Integer):
        icon = icons.int
    elif isinstance(descriptor, Number):
        icon = icons.float
    elif isinstance(descriptor, Bool):
        icon = icons.boolean

    return icon
