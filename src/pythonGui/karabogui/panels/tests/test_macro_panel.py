from karabo.common.project.macro import MacroModel
from karabogui.panels.macropanel import MacroPanel


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
