# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtWidgets import QMessageBox

from karabo.common.scenemodel.api import DisplayCommandModel
from karabogui.controllers.display.command import DisplayCommand
from karabogui.dialogs.api import ProxiesDialog
from karabogui.sceneview.widget.api import ControllerContainer
from karabogui.testing import click_button


def test_proxies_dialog(gui_app, mocker):
    model = DisplayCommandModel(keys=["TestDevice.hello", "TestDevice.bye"])
    container = ControllerContainer(DisplayCommand, model, None)

    controller = container.widget_controller
    assert controller.widget is not None
    assert len(controller._actions) == 2

    assert container.toolTip() == "TestDevice.hello, TestDevice.bye"
    dialog = ProxiesDialog(container)
    mbox = mocker.patch("karabogui.dialogs.proxies_dialog.QMessageBox")
    mbox.return_value = QMessageBox.Yes
    view = dialog.ui_list_view
    model = view.model()
    view.setCurrentIndex(model.index(0, 0))
    click_button(dialog.ui_remove_button)
    # A button is removed
    assert len(controller._actions) == 1
    assert container.toolTip() == "TestDevice.hello"
    container.destroy()
