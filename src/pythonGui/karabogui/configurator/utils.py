# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import json
from enum import Enum

from qtpy.QtCore import QMimeData, Qt
from qtpy.QtGui import QColor, QPalette
from qtpy.QtWidgets import QStyle

import karabogui.access as krb_access
from karabo.common.api import (
    KARABO_EDITABLE_ATTRIBUTES, KARABO_SCHEMA_DAQ_POLICY,
    KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, KARABO_SCHEMA_UNIT_SYMBOL, State)
from karabo.native import AccessMode, Assignment
from karabogui import icons
from karabogui.binding.api import (
    BindingRoot, BoolBinding, CharBinding, ChoiceOfNodesBinding, FloatBinding,
    ImageBinding, IntBinding, ListOfNodesBinding, NodeBinding, StringBinding,
    VectorHashBinding, WidgetNodeBinding, get_binding_value, get_editor_value)
from karabogui.controllers.api import get_compatible_controllers
from karabogui.indicators import (
    ERROR_COLOR_ALPHA, OK_COLOR, UNKNOWN_COLOR_ALPHA)
from karabogui.itemtypes import ConfiguratorItemType

# The fixed height of rows in the configurator
FIXED_ROW_HEIGHT = 30
RECURSIVE_BINDING = (BindingRoot, ChoiceOfNodesBinding, NodeBinding)


class ButtonState(Enum):
    PRESSED = QStyle.State_Enabled | QStyle.State_Sunken
    ENABLED = QStyle.State_Enabled | QStyle.State_Raised | QStyle.State_Off
    DISABLED = QStyle.State_On


def _get_item_type(binding):
    """Get the item type for a `binding`

    We can only drag nodes and leaf elements
    """
    if isinstance(binding, NodeBinding):
        return ConfiguratorItemType.NODE
    return ConfiguratorItemType.LEAF


def dragged_configurator_items(proxies):
    """Create a QMimeData object containing items dragged from the configurator
    """
    dragged = []
    for proxy in proxies:
        if proxy.binding is None:
            continue

        # Collect the relevant information
        binding = proxy.binding
        item_type = _get_item_type(binding)
        default_name = proxy.path.split('.')[-1]
        data = {
            'key': proxy.key,
            'label': binding.displayed_name or default_name,
            'type': item_type,
        }

        factories = get_compatible_controllers(binding, can_edit=False)
        if factories:
            data['display_widget_class'] = factories[0].__name__
        if binding.access_mode is AccessMode.RECONFIGURABLE:
            factories = get_compatible_controllers(binding, can_edit=True)
            if factories:
                data['edit_widget_class'] = factories[0].__name__
        # Add it to the list of dragged items
        dragged.append(data)

    if not dragged:
        return None

    mimeData = QMimeData()
    mimeData.setData('source_type', bytearray('ParameterTreeWidget',
                                              encoding='UTF-8'))
    mimeData.setData('tree_items', bytearray(json.dumps(dragged),
                                             encoding='UTF-8'))
    return mimeData


def get_child_names(proxy):
    """Return all the names of a proxy's accessible children.

    In the case of a `BaseDeviceProxy` binding, this is a list of properties.
    For others, this is a list of attribute names.
    """
    ret = []
    level = krb_access.GLOBAL_ACCESS_LEVEL

    binding = proxy.binding
    if isinstance(binding, RECURSIVE_BINDING):
        ret = binding.children_names.get(level, [])
        # lazily cache visible children names
        if len(ret) == 0:
            for name in binding.value:
                node = getattr(binding.value, name)
                if node.required_access_level <= level:
                    ret.append(name)
            binding.children_names[level] = ret
    else:
        # Use a lazy cache on the proxy
        ret = proxy.editable_attributes
        if len(ret) == 0:
            ret = _get_editable_attributes(binding)
            proxy.editable_attributes = ret

    return ret


def is_mandatory(binding):
    """Retrieves if a binding or a binding within a node is mandatory

    This function is solely used for DeviceClass proxies
    """
    if binding.assignment is Assignment.MANDATORY:
        return True
    elif isinstance(binding, RECURSIVE_BINDING):
        ret = False

        def recurse(binding):
            """Walk through a node binding for mandatory parameters
            """
            nonlocal ret
            for name in binding.value:
                node = getattr(binding.value, name)
                if isinstance(node, RECURSIVE_BINDING):
                    recurse(node)
                elif node.assignment is Assignment.MANDATORY:
                    ret = True
                    break

        recurse(binding)
        return ret

    return False


def get_device_state_string(device_proxy):
    """Return a device state as a STRING to be checked with
    BaseBinding.is_allowed.

    NOTE: BaseBinding.is_allowed is tolerant of empty string states because it
    does not try to make the passed in string a `State` instance. This is an
    important property!
    """
    state_binding = device_proxy.state_binding
    if state_binding is None:
        return ''
    return get_binding_value(state_binding, '')


def get_device_locked_string(device_proxy):
    """Return a check if a device proxy is locked!
    """
    locked_binding = device_proxy.locked_binding
    if locked_binding is None:
        return ''
    return get_binding_value(locked_binding, '')


def get_icon(binding):
    """Get the proper icon to show next to a property in the configurator
    """
    if len(binding.options) > 0:
        return icons.enum

    icon = icons.undefined
    if isinstance(binding, (CharBinding, StringBinding)):
        icon = icons.string
    elif isinstance(binding, IntBinding):
        icon = icons.int
    elif isinstance(binding, FloatBinding):
        icon = icons.float
    elif isinstance(binding, BoolBinding):
        icon = icons.boolean
    elif isinstance(binding, (ImageBinding, WidgetNodeBinding)):
        icon = icons.image

    return icon


def get_attr_icon(binding, name):
    """Get the proper attribute icon to show next to an attribute
    """
    if name in (KARABO_SCHEMA_DAQ_POLICY, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
                KARABO_SCHEMA_UNIT_SYMBOL):
        return icons.enumAttribute
    icon = icons.undefinedAttribute
    if isinstance(binding, IntBinding):
        icon = icons.intAttribute
    elif isinstance(binding, FloatBinding):
        icon = icons.floatAttribute
    elif isinstance(binding, BoolBinding):
        icon = icons.booleanAttribute
    elif isinstance(binding, (StringBinding, CharBinding)):
        icon = icons.stringAttribute

    return icon


def get_proxy_value(index, proxy, is_edit_col=False):
    """Return the actual value of the given `proxy`, depending on whether this
    is requested for an editable column
    """
    is_editable = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
    if is_edit_col and not is_editable:
        return ''

    binding = proxy.binding
    if isinstance(binding, ChoiceOfNodesBinding):
        return binding.choice or ''
    if isinstance(binding, ListOfNodesBinding):
        # Use the class_id to represent each node in the ListOfNodes
        val = [node.class_id for node in get_binding_value(binding, [])]
        return val
    if isinstance(binding, (BindingRoot, NodeBinding, VectorHashBinding)):
        return ''

    value = _proxy_value(proxy, is_edit_col)
    if isinstance(value, (bytes, bytearray)):
        return ''

    return value


def handle_default_state(allowed, state):
    """Determine the resting state of a given box's button.
    """
    if allowed and state != ButtonState.PRESSED:
        state = ButtonState.ENABLED
    if not allowed:
        state = ButtonState.DISABLED
    return state


def set_fill_rect(painter, option, index):
    """Update the rectangle of the given `painter` depending on the given
    `options`
    """
    if option.state & QStyle.State_Selected:
        if option.state & QStyle.State_Active:
            painter.fillRect(option.rect, option.palette.highlight())
        elif not (option.state & QStyle.State_HasFocus):
            # XXX: Palette background is deprecated in Qt5. Take the brush
            # with background role
            painter.fillRect(option.rect,
                             option.palette.brush(QPalette.Background))
    else:
        brush = index.data(Qt.BackgroundRole)
        if brush is not None:
            painter.fillRect(option.rect, brush)


# ----------------------------------------------------------------------------
# Private details

def _get_editable_attributes(binding):
    """Return the editable attribute names of a binding
    """
    names = []
    attributes = binding.attributes
    for name in KARABO_EDITABLE_ATTRIBUTES:
        value = attributes.get(name)
        if value is not None:
            # Skip blank units
            if name == KARABO_SCHEMA_UNIT_SYMBOL and value == '':
                continue
            # Skip metric prefixes with no associated units
            if (name == KARABO_SCHEMA_METRIC_PREFIX_SYMBOL and value == ''
                    and attributes.get(KARABO_SCHEMA_UNIT_SYMBOL, '') == ''):
                continue
            names.append(name)
    return tuple(names)


def _proxy_value(proxy, is_edit_col):
    """If a value is needed from the editable column, then get the value from
    the binding. This is in case the user has made a change which was not
    yet applied.
    """
    if is_edit_col:
        return get_editor_value(proxy, '')
    return get_binding_value(proxy, '')


def threshold_triggered(value, limit_low, limit_high):
    """Check if a value exceeds the limits low or high

    This method is typically used to check alarms and warnings
    """
    if ((limit_low is not None and value < limit_low) or
            (limit_high is not None and value > limit_high)):
        return True
    return False


_STATE_QCOLOR_ALPHA = {
    State.ERROR.name: QColor(*ERROR_COLOR_ALPHA),
    State.UNKNOWN.name: QColor(*UNKNOWN_COLOR_ALPHA)
}

_STATE_QCOLOR_DEFAULT = QColor(*OK_COLOR)


def get_qcolor_state(state):
    """Get the opague background color according to a `state` string
    for the configurator
    """
    return _STATE_QCOLOR_ALPHA.get(state, _STATE_QCOLOR_DEFAULT)
