#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on December 7, 2016
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
from qtpy.QtCore import QObject

from karabo.common.api import WeakMethodRef


class SystemTopologyListener(QObject):
    """ An object which simply listens to KaraboBroadcastEvents concerning the
    system topology and passes them along via a callback method.
    """
    def __init__(self, notification_cb, parent=None):
        super().__init__(parent)
        # Hold a weak reference to a callback bound method
        self.callback = WeakMethodRef(notification_cb, num_args=2)

    def _event_topology(self, data):
        """ Router for incoming broadcasts
        """
        self.callback(data['devices'], data['servers'])
