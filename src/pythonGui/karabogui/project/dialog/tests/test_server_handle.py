# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology

from ..server_handle import ServerHandleDialog


def test_server_handle_dialog(gui_app):
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        dialog = ServerHandleDialog()
        assert dialog.ui_server_id.count() == 1
        assert dialog.server_id == "swerver"
        dialog.done(0)
