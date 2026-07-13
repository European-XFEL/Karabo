from karabogui.project.dialog import project_handle


def test_domain_change(gui_app, mocker):

    dialog = project_handle.LoadProjectDialog()
    mock_update = mocker.patch.object(dialog, "update_view")
    mock_update.reset()
    dialog.cbDomain.currentIndexChanged.emit(2)
    assert mock_update.call_count == 1
