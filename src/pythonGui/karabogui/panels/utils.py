# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op

from karabogui import icons

from .alarmpanel import AlarmPanel
from .loggingpanel import LoggingPanel
from .macropanel import MacroPanel
from .scenepanel import ScenePanel
from .scriptingpanel import ScriptingPanel

PANEL_ICONS = {
    AlarmPanel: icons.alarmWarning,
    LoggingPanel: icons.logMenu,
    ScriptingPanel: icons.consoleMenu,
    MacroPanel: icons.edit,
    ScenePanel: icons.image
}


def get_panel_icon(panel):
    return PANEL_ICONS.get(type(panel))


def format_vector_hash_details(binding, value):
    if value is None:
        return "Schema is not followed"

    details = []
    for index, row_hash in enumerate(value):
        if row_hash is None:
            continue
        row_details = []
        for row_name, row_value in row_hash.items():
            row_binding = binding.bindings.get(row_name)
            detail = format_property_details(row_binding, row_name, row_value)
            row_details.append(detail)
        # Do some formatting, a la yaml.
        tabbed_details = ["    {}".format(detail) for detail in row_details]
        tabbed_row = ["    {}".format(deets)
                      for deets in [f"row {index}:"] + tabbed_details]
        details.append("\n".join(tabbed_row))
    return "\n" + "\n".join(details)


def format_property_details(binding, path, value=''):
    name = binding.displayed_name or path
    data_type = type(binding).__name__[:-len('binding')]
    return f"- {name} ({data_type}): {value}"


def compare_proxy_essential(old, new):
    """Compare the necessary proxy information and return True if equal

    Note: Proxies might change their classes, e.g. are different from project
    and online information. Hence, they are not considered.

    :returns: Boolean if essentials are equal (serverId, deviceId)
    """
    def get_proxy_id(proxy):
        server_id = proxy.server_id
        device_id = getattr(proxy, "device_id", "")
        return tuple([server_id, device_id])

    return get_proxy_id(old) == get_proxy_id(new)


def get_panel_ui(ui_name):
    assert ui_name.endswith(".ui"), f"{ui_name} is not a `.ui` file"

    return op.join(op.abspath(op.dirname(__file__)), "ui", ui_name)
