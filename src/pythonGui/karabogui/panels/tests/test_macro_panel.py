from qtpy.QtWidgets import QDialog

from karabo.common.project.macro import MacroModel
from karabogui.panels.macropanel import MacroPanel
from karabogui.singletons.configuration import Configuration
from karabogui.testing import singletons


def test_save_dialog(gui_app, mocker):
    """Test the default file name on saving macro from panel"""

    # Name without slash
    model = MacroModel(simple_name="macro_foo")
    panel = MacroPanel(model=model)

    # To avoid actually writing the file mocking the 'open' function.
    mock_method = mocker.patch("builtins.open") # noqa

    mock_dialog = mocker.patch(
        'karabogui.panels.macropanel.getSaveFileName', return_value="foo.py")
    panel.on_save()
    assert mock_dialog.call_args.kwargs['selectFile'] == "macro_foo.py"

    # Name with slash
    panel.model.simple_name = "macro/foo"
    panel.on_save()
    assert mock_dialog.call_args.kwargs['selectFile'] == "macro_foo.py"


def test_on_debug_run(gui_app, mocker):
    """
    Test that 'on_debug_run' shows the 'DebugRunDialog' and runs the macro on
    specific server and set the configuration when the dialog is accepted.
    """
    model = MacroModel(simple_name="macro_foo")
    panel = MacroPanel(model=model)

    mock_run = mocker.patch("karabogui.panels.macropanel.run_macro")
    mock_dialog = mocker.patch("karabogui.panels.macropanel.DebugRunDialog")

    mock_dialog().exec.return_value = QDialog.Rejected
    panel.on_debug_run()
    assert mock_run.call_count == 0

    config = Configuration()
    with singletons(configuration=config):
        mock_dialog().exec.return_value = QDialog.Accepted
        mock_dialog().comboBox.currentText.return_value = "DummyMacroServer"
        panel.on_debug_run()
        mock_run.assert_called_once_with(
            model, serverId="DummyMacroServer")
        assert config['macro_development'] == "DummyMacroServer"
