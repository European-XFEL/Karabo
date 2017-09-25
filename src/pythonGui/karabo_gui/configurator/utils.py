from enum import Enum
import json

from PyQt4.QtCore import QMimeData, Qt
from PyQt4.QtGui import QStyle

from karabo.middlelayer import AccessMode, Bool, Char, Integer, Number, String
import karabo_gui.icons as icons
from karabo_gui.schema import (
    ChoiceOfNodes, Dummy, ListOfNodes, Schema, VectorHash)
from karabo_gui.singletons.api import get_topology
from karabo_gui.widget import DisplayWidget, EditableWidget

# The fixed height of rows in the configurator
FIXED_ROW_HEIGHT = 30


class ButtonState(Enum):
    PRESSED = QStyle.State_Enabled | QStyle.State_Sunken
    ENABLED = QStyle.State_Enabled | QStyle.State_Raised | QStyle.State_Off
    DISABLED = QStyle.State_On


def dragged_configurator_items(boxes):
    """Create a QMimeData object containing items dragged from the configurator
    """
    def getDeviceBox(box):
        """Return a box that belongs to an active device

        if the box already is part of a running device, return it,
        if it is from a class in a project, return the corresponding
        instantiated device's box.
        """
        if box.configuration.type == "projectClass":
            topology = get_topology()
            return topology.get_device(box.configuration.id).getBox(box.path)
        return box

    dragged = []
    for box in boxes:
        # Get the box. "box" is in the project, "realbox" the
        # one on the device. They are the same if not from a project
        realbox = getDeviceBox(box)
        if realbox.descriptor is not None:
            box = realbox

        # Collect the relevant information
        data = {
            'key': box.key(),
            'label': box.descriptor.displayedName,
        }

        factory = DisplayWidget.getClass(box)
        if factory is not None:
            data['display_widget_class'] = factory.__name__
        if box.descriptor.accessMode == AccessMode.RECONFIGURABLE:
            factory = EditableWidget.getClass(box)
            if factory is not None:
                data['edit_widget_class'] = factory.__name__
        # Add it to the list of dragged items
        dragged.append(data)

    if not dragged:
        return None

    mimeData = QMimeData()
    mimeData.setData('source_type', 'ParameterTreeWidget')
    mimeData.setData('tree_items', json.dumps(dragged))
    return mimeData


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


def get_box_value(index, box, is_edit_col=False):
    """Return the actual value of the given `box`, depending on whether this is
    requested for an editable column
    """
    is_editable = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
    if is_edit_col and not is_editable:
        return ''

    descriptor = box.descriptor
    if isinstance(descriptor, (ChoiceOfNodes, ListOfNodes)):
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
