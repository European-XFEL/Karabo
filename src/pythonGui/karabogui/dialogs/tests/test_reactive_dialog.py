import json
from ast import literal_eval
from datetime import datetime, timezone

from qtpy.QtCore import Qt
from qtpy.QtNetwork import QNetworkReply, QNetworkRequest

from karabogui import access as krb_access
from karabogui.const import IS_WINDOWS_SYSTEM
from karabogui.dialogs.reactive_login_dialog import (
    AccessCodeWidget, LoginType, ReactiveLoginDialog, UserSessionDialog,
    remaining_time_info)
from karabogui.singletons.configuration import Configuration
from karabogui.testing.utils import click_button, keySequence, singletons


def test_access_level(gui_app):
    configuration = Configuration()

    with singletons(configuration=configuration):
        dialog = ReactiveLoginDialog()
        assert dialog.access_level == (
            krb_access.GLOBAL_ACCESS_LEVEL.name.lower())

        # Non-authenticated login type, access level from the dialog.
        dialog.login_type = LoginType.ACCESS_LEVEL
        assert dialog.combo_access_level.currentText() == "expert"
        assert dialog.access_level == "expert"

        # For authenticated login type, access level from configuration.
        configuration["access_level"] = "operator"
        dialog.login_type = LoginType.REFRESH_TOKEN
        assert dialog.access_level == "operator"


def test_temporarySessionDialog(gui_app, mocker):
    dialog = UserSessionDialog()
    assert not dialog.ok_button.isEnabled()
    access_code = "123456"
    hostname = "karabo.xfel.eu"
    for i, cell in enumerate(dialog.edit_access_code.cells, start=1):
        cell.setText(str(i))
    mocker.patch("karabogui.dialogs.reactive_login_dialog.CLIENT_HOST",
                 new=hostname)
    dialog.access_manager = mocker.Mock()
    dialog._login_authenticated()
    assert dialog.access_manager.post.call_count == 1
    call_args, _ = dialog.access_manager.post.call_args
    network_req, args = call_args
    assert isinstance(network_req, QNetworkRequest)
    args_dict = json.loads(args)
    assert args_dict["access_code"] == int(access_code)
    assert not args_dict["remember_login"]
    assert args_dict["client_hostname"] == hostname


def test_switch_user(gui_app, mocker):
    krb_access.SESSION_END_NOTICE = True
    dialog = UserSessionDialog()
    assert not dialog.ok_button.isEnabled()
    access_code = "123456"
    hostname = "karabo.xfel.eu"
    for i, cell in enumerate(dialog.edit_access_code.cells, start=1):
        cell.setText(str(i))
    mocker.patch("karabogui.dialogs.reactive_login_dialog.CLIENT_HOST",
                 new=hostname)
    dialog.access_manager = mocker.Mock()
    assert dialog.combo_mode.currentIndex() == 1
    dialog.remember_login.setChecked(True)
    dialog._login_authenticated()
    assert dialog.access_manager.post.call_count == 1
    call_args, _ = dialog.access_manager.post.call_args
    network_req, args = call_args
    assert isinstance(network_req, QNetworkRequest)
    args_dict = json.loads(args)
    assert args_dict["access_code"] == int(access_code)
    assert args_dict["remember_login"]
    assert args_dict["client_hostname"] == hostname

    dialog.remember_login.setChecked(False)
    dialog._login_authenticated()
    assert dialog.access_manager.post.call_count == 2
    call_args, _ = dialog.access_manager.post.call_args
    network_req, args = call_args
    assert isinstance(network_req, QNetworkRequest)
    args_dict = json.loads(args)
    assert args_dict["access_code"] == int(access_code)
    assert not args_dict["remember_login"]
    assert args_dict["client_hostname"] == hostname
    krb_access.SESSION_END_NOTICE = False


def test_connect_button(gui_app):
    dialog = ReactiveLoginDialog()
    assert not dialog.connect_button.isEnabled()
    dialog.login_type = LoginType.REFRESH_TOKEN
    dialog._update_button()
    assert dialog.connect_button.isEnabled()

    dialog._on_switch_user_click()
    assert not dialog.connect_button.isEnabled()


def test_refresh_token(gui_app, mocker):
    configuration = Configuration()
    with singletons(configuration=configuration):
        configuration["refresh_token"] = "dummy_token"
        dialog = ReactiveLoginDialog()
        dialog.login_type = LoginType.REFRESH_TOKEN
        mocked_method = mocker.patch.object(
            dialog, "_refresh_authentication")
        dialog.connect_clicked()
        assert mocked_method.call_count


def test_refresh_authentication(gui_app, mocker):
    """With REFRESH_TOKEN, the login dialog should call the correct post
    request."""
    configuration = Configuration()
    with singletons(configuration=configuration):
        configuration["refresh_token"] = "dummy_token"
        configuration["refresh_token_user"] = "karabo"
        dialog = ReactiveLoginDialog()
        dialog.login_type = LoginType.REFRESH_TOKEN
        mocked_access_manger = mocker.patch.object(dialog, "access_manager")
        dialog._refresh_authentication()
        assert mocked_access_manger.post.call_count == 1

        args = mocked_access_manger.post.call_args[0]
        network, args = args

        assert isinstance(network, QNetworkRequest)
        url = network.url().url()
        assert "refresh_tokens" in url

        args = literal_eval(args.decode())
        assert len(args) == 3
        assert args.get("refresh_token") == "dummy_token"


def test_access_code_login(gui_app, mocker):
    """Verify the correct request is posted when loging with an access code
    even when refresh token is already present."""
    configuration = Configuration()
    with singletons(configuration=configuration):
        configuration["refresh_token"] = "dummy_token"
        dialog = ReactiveLoginDialog()
        dialog.login_type = LoginType.REFRESH_TOKEN
        dialog._update_button()
        assert dialog.connect_button.isEnabled()

        click_button(dialog.switch_button)
        assert dialog.login_type == LoginType.USER_AUTHENTICATED
        assert not dialog.connect_button.isEnabled()

        for i, cell in enumerate(dialog.edit_access_code.cells, start=1):
            cell.setText(str(i))

        dialog._update_button()
        assert dialog.connect_button.isEnabled()
        post_access_code = mocker.patch.object(dialog, "_login_authenticated")
        post_refresh_token = mocker.patch.object(
            dialog, "_refresh_authentication")
        click_button(dialog.connect_button)
        assert post_refresh_token.call_count == 0
        assert post_access_code.call_count == 1


def test_authReply(gui_app, mocker):
    configuration = Configuration()
    with singletons(configuration=configuration):
        dialog = ReactiveLoginDialog()
        reply = mocker.Mock(spec=QNetworkReply)
        reply.error.return_value = QNetworkReply.NoError

        failure = (
            b'{"success":false,"once_token":null,"refresh_token":null,'
            b'"username":null,"error_msg":"No token for access code 444444"}')
        reply.readAll.return_value = failure
        dialog.onAuthReply(reply)
        assert dialog.label_status.text() == "No token for access code 444444"

        success = (b'{"success":true,"once_token":"abc","refresh_token":"xyz",'
                   b'"username":"karabo","error_msg":null}')
        reply.readAll.return_value = success
        dialog.onAuthReply(reply)
        assert krb_access.ONE_TIME_TOKEN == "abc"
        assert configuration["refresh_token"] == "xyz"


def test_access_widget(gui_app):
    widget = AccessCodeWidget()
    assert widget.get_access_code() == ""
    assert widget.objectName() == "AccessCodeWidget"
    for i, cell in enumerate(widget.cells):
        assert cell.objectName() == f"Cell_{i}"
    widget.cells[0].setText("4")
    assert widget.get_access_code() == "4"
    assert widget.focusWidget() == widget.cells[1]

    widget.cells[3].setText("123456")
    assert widget.get_access_code() == "123456"
    assert widget.cells[0].text() == "1"
    assert widget.cells[1].text() == "2"
    assert widget.cells[2].text() == "3"
    assert widget.cells[3].text() == "4"
    assert widget.cells[4].text() == "5"
    assert widget.cells[5].text() == "6"

    # Backspace should clear the cell
    cell = widget.focusWidget()
    current_index = widget.cells.index(cell)
    assert current_index
    assert cell.text()
    keySequence(cell, Qt.Key_Backspace)
    new_cell = widget.focusWidget()
    new_index = widget.cells.index(new_cell)
    assert not cell.text()
    assert new_index == current_index

    # Backspace should move focus to previous cell from empty cell.
    keySequence(cell, Qt.Key_Backspace)
    new_cell = widget.focusWidget()
    new_index = widget.cells.index(new_cell)
    assert new_index == current_index - 1

    # Backspace when focus is on first cell.
    first_cell = widget.cells[0]
    first_cell.setFocus(True)
    keySequence(first_cell, Qt.Key_Backspace)
    assert first_cell == widget.focusWidget()

    cells = widget.cells
    for _ in range(7):
        cell = widget.focusWidget()
        cell_index = cells.index(cell)
        keySequence(cell, Qt.Key_Right)
        if cell_index < len(cells) - 1:
            assert cells[cell_index + 1] == widget.focusWidget()
        else:
            assert cells[cell_index] == widget.focusWidget()

    for _ in range(7):
        cell = widget.focusWidget()
        cell_index = cells.index(cell)
        keySequence(cell, Qt.Key_Left)
        if cell_index == 0:
            assert cells[cell_index] == widget.focusWidget()
        else:
            assert cells[cell_index - 1] == widget.focusWidget()

    # Backspace deletes the number when cursor is on its right.
    fourth_cell = cells[4]
    third_cell = cells[3]
    fourth_cell.setFocus(True)
    fourth_cell.setCursorPosition(1)
    keySequence(fourth_cell, Qt.Key_Backspace)
    assert not bool(fourth_cell.text())
    assert fourth_cell == widget.focusWidget()

    # Backspace switches to previous cell if the cursor is on left.
    third_cell.setFocus(True)
    third_cell.setCursorPosition(0)
    keySequence(third_cell, Qt.Key_Backspace)
    assert bool(third_cell.text())
    assert cells[2] == widget.focusWidget()


def test_session_dialog(mocker):
    """Test the session dialog and the remaining time"""
    def create_start_str() -> str:
        """Generates a UTC datetime string in expected format"""
        now_utc = datetime.now(timezone.utc)
        return now_utc.strftime("%Y%m%dT%H%M%S.%fZ")

    # 30 minutes from now
    start = create_start_str()
    duration = 30 * 60  # 30 minutes
    _, _, remaining = remaining_time_info(start, duration)
    if IS_WINDOWS_SYSTEM:
        assert "~30 minute(s) left" in remaining
    else:
        assert "~29 minute(s) left" in remaining

    # 2 hours from now
    start = create_start_str()
    duration = 2 * 3600
    _, _, remaining = remaining_time_info(start, duration)
    assert "~2.00 hour(s)" in remaining

    # 3 days from now
    start = create_start_str()
    duration = 3 * 86400
    _, _, remaining = remaining_time_info(start, duration)
    assert "~3.00 day" in remaining

    # 1 hours ago
    start = create_start_str()
    duration = -3600
    _, _, remaining = remaining_time_info(start, duration)
    assert remaining == "Session ended"

    # test the dialog
    network = mocker.Mock()
    with singletons(network=network):
        dialog = UserSessionDialog()
        network.onGetGuiSessionInfo.assert_called()
        start_time = create_start_str()
        duration = 2 * 3600
        data = {"sessionStartTime": start_time, "sessionDuration": duration}
        dialog._event_user_session_info(data)
        assert dialog.info_remaining_time.text() == "~2.00 hour(s) left"
