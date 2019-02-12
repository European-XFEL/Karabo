#############################################################################
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.navigation.device_view import DeviceTreeView

from .base import BasePanelWidget


class DevicePanel(BasePanelWidget):
    def __init__(self):
        super(DevicePanel, self).__init__("DeviceTopology")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        self.device_view = DeviceTreeView(widget)

        main_layout.addWidget(self.device_view)
        return widget
