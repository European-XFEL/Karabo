# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAbstractItemView

from karabo.native import Hash
from karabogui.dialogs.guidebuginfo import GuiDebugInfo
from karabogui.testing import singletons


def test_debug_info_topology_tree(gui_app, mocker):
    network = mocker.Mock()
    mocker.patch("karabogui.dialogs.guidebuginfo.register_for_broadcasts")
    mocker.patch("karabogui.dialogs.guidebuginfo.unregister_from_broadcasts")
    with singletons(network=network):
        dialog = GuiDebugInfo()
        network.getClientDebugInfo.assert_called_once_with()
        dialog.refresh_button.click()
        assert network.getClientDebugInfo.call_count == 2
    topology = Hash(
        "device", Hash("device-1", Hash("serverId", "server-1")),
        "server", Hash("server-1", Hash()))

    dialog._event_info({"info": Hash(
        "systemTopology", topology,
        "client-address", Hash(
            "monitoredDevices", ["device-1", "device-2"],
            "pipelineConnections", ["pipe-1", "pipe-2"],
            "pipelineConnectionsReadiness", [True, False]))})

    assert dialog.model.headerData(0, Qt.Horizontal) == "Instance Type"
    assert dialog.model.headerData(1, Qt.Horizontal) == "Instance Id"
    assert dialog.topology.selectionMode() == QAbstractItemView.NoSelection
    assert dialog.topology.editTriggers() == QAbstractItemView.NoEditTriggers

    root_item = dialog.model.item(0)
    assert root_item.text() == "device"
    assert root_item.font().bold()
    assert root_item.child(0, 0).text() == ""
    assert root_item.child(0, 1).text() == "device-1"

    root_index = dialog.filter_model.mapFromSource(root_item.index())
    assert not dialog.filter_model.flags(root_index) & Qt.ItemIsEditable
    assert dialog.topology.isExpanded(root_index)
    dialog.topology.header().sectionDoubleClicked.emit(0)
    assert not dialog.topology.isExpanded(root_index)
    dialog.topology.header().sectionDoubleClicked.emit(0)
    assert dialog.topology.isExpanded(root_index)

    dialog.search_field.setText("device-1")
    assert dialog.filter_model.rowCount() == 1

    assert dialog.details_stack.count() == 3
    dialog.details_selector.setCurrentIndex(1)
    assert dialog.details_stack.currentIndex() == 1
    assert dialog.monitored_filter_model.rowCount() == 2
    dialog.monitored_search.setText("device-2")
    assert dialog.monitored_filter_model.rowCount() == 1

    dialog.details_selector.setCurrentIndex(2)
    assert dialog.details_stack.currentIndex() == 2
    assert dialog.pipeline_model.rowCount() == 2
    assert dialog.pipeline_model.item(0, 0).text() == "pipe-1"
    assert dialog.pipeline_model.item(0, 1).text() == "True"
    assert dialog.pipeline_model.item(1, 0).text() == "pipe-2"
    assert dialog.pipeline_model.item(1, 1).text() == "False"
