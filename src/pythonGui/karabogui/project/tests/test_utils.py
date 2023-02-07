from karabo.common.project.api import MacroModel
from karabogui.project.utils import run_macro


def test_confirmation_dialog(gui_app, mocker):
    """Check if the confirmation dialog shows up when the macro is not saved"""
    mocker.patch("karabogui.project.utils.get_macro_servers",
                 return_value=["dummy_server"])
    mocker.patch(
        "karabo.common.sanity_check.validate_macro", return_value=True)
    mocker.patch("karabogui.project.utils.get_topology")
    mocker.patch("karabogui.project.utils.get_network")

    # Macro not changed
    macro_model = MacroModel(modified=False)
    mock_question = mocker.patch(
        "karabogui.project.utils.QMessageBox.question")
    run_macro(macro_model)
    assert mock_question.call_count == 0

    # Macro changed and not saved
    macro_model.modified = True
    run_macro(macro_model)

    title = "Run Unsaved Macro?"
    text = (
        "The Macro is not saved in the Project. Do you really want to run "
        "Macro?")
    parent = None
    assert mock_question.call_count == 1

    args = mock_question.call_args
    arg1, arg2, arg3, _ = args[0]
    assert arg1 == parent
    assert arg2 == title
    assert arg3 == text
