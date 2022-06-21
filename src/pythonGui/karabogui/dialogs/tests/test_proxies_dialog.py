from qtpy.QtWidgets import QMessageBox

from karabo.native import Configurable, Slot
from karabogui.controllers.display.command import DisplayCommand
from karabogui.dialogs.api import ProxiesDialog
from karabogui.testing import click_button, get_property_proxy


class SlottedDevice(Configurable):

    @Slot()
    def hello(self):
        pass

    @Slot()
    def bye(self):
        pass


def get_slot_proxy():
    schema = SlottedDevice.getClassSchema()
    hello_proxy = get_property_proxy(schema, "hello")
    bye_proxy = get_property_proxy(schema, "bye")
    return hello_proxy, bye_proxy


def test_proxies_dialog(gui_app, mocker):
    proxies = get_slot_proxy()
    controller = DisplayCommand(proxy=proxies[0])
    controller.create(None)
    controller.visualize_additional_property(proxies[1])

    assert controller.widget is not None
    assert len(controller._actions) == 2

    dialog = ProxiesDialog(controller)
    mbox = mocker.patch("karabogui.dialogs.proxies_dialog.QMessageBox")
    mbox.return_value = QMessageBox.Yes
    view = dialog.ui_list_view
    model = view.model()
    view.setCurrentIndex(model.index(0, 0))
    click_button(dialog.ui_remove_button)
    # A button is removed
    assert len(controller._actions) == 1
    controller.destroy()
    assert controller.widget is None
