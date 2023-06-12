#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 7, 2019
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
#############################################################################
from abc import abstractmethod

from traits.api import ABCHasStrictTraits

from karabogui.dialogs.api import LogDialog
from karabogui.itemtypes import NavigationItemTypes
from karabogui.logger import get_logger
from karabogui.request import retrieve_default_scene
from karabogui.util import move_to_cursor, version_compatible


class NavigationHandler(ABCHasStrictTraits):
    @abstractmethod
    def can_handle(self, info):
        """Check whether the click event can be handled."""

    @abstractmethod
    def handle(self, info, parent=None):
        """Handle the click event."""


class DeviceSceneHandler(NavigationHandler):
    """Device DoubleClick for the navigation view
    """

    def can_handle(self, info):
        navigation_type = info.get('type')
        if navigation_type is NavigationItemTypes.DEVICE:
            return True
        return False

    def handle(self, info, parent=None):
        device_id = info.get("deviceId")
        retrieve_default_scene(device_id)


class ServerLogHandler(NavigationHandler):
    """Device DoubleClick for the navigation view
    """

    def can_handle(self, info):
        navigation_type = info.get("type")
        if navigation_type is NavigationItemTypes.SERVER:
            return True
        return False

    def handle(self, info, parent=None):
        server_id = info.get("serverId")
        karabo_version = info.get("karaboVersion")

        if not version_compatible(karabo_version, 2, 14):
            get_logger().info(
                f"The server <b>{server_id}</b> cannot provide logs "
                f"since its framework version ({karabo_version}) is "
                "lower than 2.14.")
            return

        widget = LogDialog(server_id, parent=parent)
        move_to_cursor(widget)
        widget.show()
        widget.raise_()
        widget.activateWindow()
