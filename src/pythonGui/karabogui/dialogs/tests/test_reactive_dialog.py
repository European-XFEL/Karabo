import json
from ast import literal_eval

from qtpy.QtNetwork import QNetworkReply, QNetworkRequest

from karabogui import access as krb_access
from karabogui.dialogs.reactive_login_dialog import (
    AccessCodeWidget, LoginType, ReactiveLoginDialog, TemporarySessionDialog)
from karabogui.testing.utils import click_button


def test_access_level(gui_app):
    dialog = ReactiveLoginDialog()
    assert dialog.access_level == krb_access.GLOBAL_ACCESS_LEVEL.name.lower()

    dialog.login_type = LoginType.ACCESS_LEVEL
    assert dialog.access_level == "admin"


def test_temporarySessionDialog(gui_app, mocker):
    dialog = TemporarySessionDialog()
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


def test_connect_button(gui_app):
    dialog = ReactiveLoginDialog()
    assert not dialog.connect_button.isEnabled()
    dialog.login_type = LoginType.REFRESH_TOKEN
    dialog._update_button()
    assert dialog.connect_button.isEnabled()

    dialog._switch_to_auth_page()
    assert not dialog.connect_button.isEnabled()


def test_refresh_token(gui_app, mocker):
    krb_access.REFRESH_TOKEN = "dummy_token"
    dialog = ReactiveLoginDialog()
    dialog.login_type = LoginType.REFRESH_TOKEN
    mocked_method = mocker.patch.object(
        dialog, "_refresh_authentication")
    dialog.connect_clicked()
    assert mocked_method.call_count


def test_refresh_authentication(gui_app, mocker):
    """With REFRESH_TOKEN, the login dialog should call the correct post
    request."""
    krb_access.REFRESH_TOKEN = "dummy_token"
    krb_access.REFRESH_TOKEN_USER = "karabo"
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
    krb_access.REFRESH_TOKEN = "dummy_token"
    dialog = ReactiveLoginDialog()
    dialog.login_type = LoginType.REFRESH_TOKEN
    dialog._update_button()
    assert dialog.connect_button.isEnabled()

    click_button(dialog.switch_button)
    assert dialog.login_type == LoginType.USER_AUTHENTICATED
    # Empty access code
    assert not dialog.connect_button.isEnabled()

    # Update all but not last, should not be connectable
    for i, cell in enumerate(dialog.edit_access_code.cells, start=1):
        if i == 6:
            i = ""
        cell.setText(str(i))
    dialog._update_button()
    assert not dialog.connect_button.isEnabled()

    # Complete access code
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
    assert krb_access.REFRESH_TOKEN == "xyz"


def test_access_widget(gui_app):
    widget = AccessCodeWidget()
    assert widget.get_access_code() == ""

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
