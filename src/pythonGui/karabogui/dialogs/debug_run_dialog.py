from qtpy import uic
from qtpy.QtWidgets import QDialog

from karabogui.dialogs.utils import get_dialog_ui
from karabogui.singletons.api import get_config


class DebugRunDialog(QDialog):
    """
    A dialog to show the list of MacroServers with 'Development' serverFlag,
    in a combobox.
    """

    def __init__(self, serverIds=None, parent=None):
        super().__init__(parent=parent)
        ui_file = get_dialog_ui("debug_run.ui")
        uic.loadUi(ui_file, self)
        button_box = self.buttonBox
        run_button = button_box.button(button_box.Ok)
        run_button.setText("Run")
        if not serverIds:
            run_button.setEnabled(False)
            run_button.setToolTip(
                "Make sure to have at least one Development MacroServer")
        else:
            self.comboBox.addItems(serverIds)

            previous_server = get_config()["macro_development"]
            if previous_server in serverIds:
                self.comboBox.setCurrentText(previous_server)
