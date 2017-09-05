from enum import Enum
from PyQt4.QtGui import QStyle

from karabo.middlelayer import AccessMode
from karabo_gui.schema import Dummy, Schema, VectorHash


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


def set_fill_rect(option, painter):
    """ Update the rectangle of the given `painter` depending on the given
    `options`
    """
    if option.state & QStyle.State_Selected:
        if option.state & QStyle.State_Active:
            painter.fillRect(option.rect, option.palette.highlight())
        elif not (option.state & QStyle.State_HasFocus):
            painter.fillRect(option.rect, option.palette.background())


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


def get_box_value(box, is_edit_col=False):
    """Return the actual value of the given `box`, depending on whether this is
    requested for an editable column
    """
    descriptor = box.descriptor
    if isinstance(descriptor, (Schema, VectorHash)):
        return ''

    value = _box_value(box, is_edit_col)
    if isinstance(value, (Dummy, bytes, bytearray)):
        return ''
    if is_edit_col:
        is_editable = (descriptor.accessMode in
                       (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        if not is_editable:
            return ''

    return value


def get_vector_col_value(cell_info, is_edit_col=False):
    """Return the actual value of the given `cell_info`, depending on whether
    this is requested for an editable column
    """
    parent_row = cell_info.parent()
    if parent_row is None:
        return ''
    parent_box = parent_row.parent()
    if parent_box is None:
        return ''
    descriptor = parent_box.descriptor
    if is_edit_col:
        is_editable = (descriptor.accessMode in
                       (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        if not is_editable:
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
