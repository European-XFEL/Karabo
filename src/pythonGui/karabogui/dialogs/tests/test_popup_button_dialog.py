from karabo.common.scenemodel.api import PopupButtonModel
from karabogui.dialogs.api import PopupButtonDialog

TEXT = "XFEL: the world's largest X-ray laser."


def test_pop_button_dialog(gui_app):
    model = PopupButtonModel(
        text=TEXT, x=0, y=0, width=100, height=100)
    dialog = PopupButtonDialog(model)
    assert dialog.leText.toPlainText() == model.text
