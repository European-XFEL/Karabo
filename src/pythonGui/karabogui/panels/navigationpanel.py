#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtWidgets import QPushButton, QVBoxLayout, QWidget

from karabo.common.api import KARABO_SCHEMA_DEFAULT_SCENE
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView
from karabogui.util import get_scene_from_server
from karabogui.singletons.api import get_config

from .base import BasePanelWidget
from .searchwidget import SearchBar


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.RemoveDaemonService: self._event_remove_daemon,
            KaraboEvent.ShowDaemonService: self._event_show_daemon,
            KaraboEvent.ShowDevice: self._event_show_device
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel."""
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(2, 2, 2, 2)

        self.tree_view = SystemTreeView(widget)
        self.sbar = SearchBar(parent=widget)
        self.daemon_button = QPushButton("Service Manager", parent=widget)
        self.daemon_button.clicked.connect(self._retrieve_service_scene)
        self.daemon_button.setVisible(False)

        model = self.tree_view.model()
        self.sbar.setModel(model)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.daemon_button)
        main_layout.addWidget(self.tree_view)

        return widget

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_access_level(self, data):
        self.sbar.reset(enable=True)

    def _event_network(self, data):
        status = data.get('status', False)
        self.sbar.reset(status)
        if not status:
            self.daemon_button.setVisible(status)
            self.close_popup_widget()

    def _event_show_daemon(self, data):
        instance_id = data.get('instanceId')
        if instance_id == get_config()['daemon_manager']:
            self.daemon_button.setVisible(True)

    def _event_remove_daemon(self, data):
        instance_id = data.get('instanceId')
        if instance_id == get_config()['daemon_manager']:
            self.daemon_button.setVisible(False)

    def _event_show_device(self, data):
        if data.get('showTopology'):
            self.panel_container.switch_to_panel(self)

    # -----------------------------------------------------------------------

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    def _retrieve_service_scene(self):
        instance_id = get_config()['daemon_manager']
        get_scene_from_server(instance_id, KARABO_SCHEMA_DEFAULT_SCENE)
