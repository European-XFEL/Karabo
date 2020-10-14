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


def format_property_details(binding, path, value=''):
    name = binding.displayed_name or path
    data_type = type(binding).__name__[:-len('binding')]
    return f"- {name} ({data_type}): {value}"
