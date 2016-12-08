#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on December 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QObject

from karabo_gui.events import KaraboBroadcastEvent, KaraboEventSender
from .utils import WeakMethodRef


class SystemTopologyListener(QObject):
    """ An object which simply listens to KaraboBroadcastEvents concerning the
    system topology and passes them along via a callback method.
    """
    def __init__(self, notification_cb, parent=None):
        super(SystemTopologyListener, self).__init__(parent)
        # Hold a weak reference to a callback bound method
        self.callback = WeakMethodRef(notification_cb, num_args=2)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.SystemTopologyUpdate:
                added = event.data.get('devices-added', [])
                removed = event.data.get('devices-removed', [])
                self.callback(added, removed)
            return False
        return super(SystemTopologyListener, self).eventFilter(obj, event)
