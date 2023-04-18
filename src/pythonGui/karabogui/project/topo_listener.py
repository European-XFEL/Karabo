#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on December 7, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import QObject

from karabo.common.api import WeakMethodRef


class SystemTopologyListener(QObject):
    """ An object which simply listens to KaraboBroadcastEvents concerning the
    system topology and passes them along via a callback method.
    """
    def __init__(self, notification_cb, parent=None):
        super(SystemTopologyListener, self).__init__(parent)
        # Hold a weak reference to a callback bound method
        self.callback = WeakMethodRef(notification_cb, num_args=2)

    def _event_topology(self, data):
        """ Router for incoming broadcasts
        """
        self.callback(data['devices'], data['servers'])
