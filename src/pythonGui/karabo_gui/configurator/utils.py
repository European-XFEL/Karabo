from enum import Enum
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QStyle

from karabo.middlelayer import AccessMode, Bool, Char, Integer, Number, String
import karabo_gui.icons as icons
from karabo_gui.schema import ChoiceOfNodes, Dummy, Schema, VectorHash

# The fixed height of rows in the configurator
FIXED_ROW_HEIGHT = 30


class ButtonState(Enum):
    PRESSED = QStyle.State_Enabled | QStyle.State_Sunken
    ENABLED = QStyle.State_Enabled | QStyle.State_Raised | QStyle.State_Off
    DISABLED = QStyle.State_On


def handle_default_state(allowed, state):
    """Determine the resting state of a given box's button.
    """
    if allowed and state != ButtonState.PRESSED:
        state = ButtonState.ENABLED
    if not allowed:
        state = ButtonState.DISABLED
    return state


def set_fill_rect(painter, option, index):
    """ Update the rectangle of the given `painter` depending on the given
    `options`
    """
    if option.state & QStyle.State_Selected:
        if option.state & QStyle.State_Active:
            painter.fillRect(option.rect, option.palette.highlight())
        elif not (option.state & QStyle.State_HasFocus):
            painter.fillRect(option.rect, option.palette.background())
    else:
        brush = index.data(Qt.BackgroundRole)
        if brush is not None:
            painter.fillRect(option.rect, brush)


def get_attribute_data(attr_info, row):
    """Return the `name`, the `descriptor` and the actual value of the given
    `attr_info`
    """
    name = attr_info.names[row]
    box = attr_info.parent()
    if box is None:
        return name, None, ''
    descriptor = box.descriptor
    return name, descriptor, getattr(descriptor, name)


def get_box_value(index, box, is_edit_col=False):
    """Return the actual value of the given `box`, depending on whether this is
    requested for an editable column
    """
    is_editable = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
    if is_edit_col and not is_editable:
        return ''

    descriptor = box.descriptor
    if isinstance(descriptor, ChoiceOfNodes):
        return box.current or ''
    if isinstance(descriptor, (Schema, VectorHash)):
        return ''

    value = _box_value(box, is_edit_col)
    if isinstance(value, (Dummy, bytes, bytearray)):
        return ''

    return value


def get_icon(descriptor):
    """Get the proper icon to show next to a property in the configurator
    """
    options = getattr(descriptor, 'options', None)
    if options is not None:
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


def get_vector_col_value(index, cell_info, is_edit_col=False):
    """Return the actual value of the given `cell_info`, depending on whether
    this is requested for an editable column
    """
    is_editable = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
    if is_edit_col and not is_editable:
        return ''

    parent_row = cell_info.parent()
    if parent_row is None:
        return ''
    parent_box = parent_row.parent()
    if parent_box is None:
        return ''

    value = _box_value(parent_box, is_edit_col)
    row = value[parent_box.rowsInfo.index(parent_row)]
    return row[cell_info.name]


def _box_value(box, is_edit_col):
    """If a value is needed from the editable column, then get the value via
    the configuration. This is in case the user has made a change which was not
    yet applied.
    """
    if is_edit_col:
        return box.configuration.getUserValue(box)
    return box.value
