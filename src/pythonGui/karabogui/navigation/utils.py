import karabogui.icons as icons
from karabo.common.api import ServerFlags

_SERVER_STABLE = {
    "python": icons.python,
    "bound": icons.bound,
    "cpp": icons.cpp,
    "macro": icons.macro,
}

_SERVER_DEVELOPMENT = {
    "python": icons.pythonDevelopment,
    "bound": icons.boundDevelopment,
    "cpp": icons.cppDevelopment,
    "macro": icons.macroDevelopment,
}


def get_language_icon(node):
    """Return the appropriate language (`API`) icon from a `SystemTreeNode`
    """
    attrs = node.attributes
    language = attrs.get("lang", "unknown")
    develop = (attrs.get("serverFlags", 0)
               & ServerFlags.Development == ServerFlags.Development)

    icon_dict = _SERVER_DEVELOPMENT if develop else _SERVER_STABLE
    return icon_dict.get(language, None)
