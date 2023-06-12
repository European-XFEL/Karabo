# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import karabogui.icons as icons
from karabo.common.api import ServerFlags


def get_language_icon(node):
    """Return the appropriate language (`API`) icon from a `SystemTreeNode`
    """
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
    attrs = node.attributes
    language = attrs.get("lang", "unknown")
    develop = (attrs.get("serverFlags", 0)
               & ServerFlags.Development == ServerFlags.Development)

    icon_dict = _SERVER_DEVELOPMENT if develop else _SERVER_STABLE
    return icon_dict.get(language, None)
