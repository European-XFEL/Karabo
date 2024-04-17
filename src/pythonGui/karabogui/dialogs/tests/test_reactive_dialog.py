from ast import literal_eval

from qtpy.QtNetwork import QNetworkRequest

from karabogui import access as krb_access
from karabogui.dialogs.reactive_login_dialog import (
    EscalationDialog, LoginType, ReactiveLoginDialog)


def test_access_level(gui_app):
    dialog = ReactiveLoginDialog()
    assert dialog.access_level == krb_access.GLOBAL_ACCESS_LEVEL.name.lower()

    dialog.login_type = LoginType.ACCESS_LEVEL
    assert dialog.access_level == "admin"


def test_init(gui_app, mocker):
    dialog = EscalationDialog(username="karabo")
    assert dialog.edit_username.text() == "karabo"
    assert dialog.error_label.text() == ""

    dialog.access_manager = mocker.Mock()
    dialog._post_auth_request()
    assert dialog.access_manager.post.call_count == 1
    call_args, _ = dialog.access_manager.post.call_args
    network_req, args = call_args
    assert isinstance(network_req, QNetworkRequest)
    user = literal_eval(args.decode()).get("user")
    assert user == "karabo"


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
        dialog, "_post_auth_request_refresh_token")
    dialog.connect_clicked()
    assert mocked_method.call_count


def test_post_auth_request_refresh_token(gui_app, mocker):
    """With REFRESH_TOKEN, the login dialog should call the correct post
    request."""
    krb_access.REFRESH_TOKEN = "dummy_token"
    dialog = ReactiveLoginDialog()
    dialog.login_type = LoginType.REFRESH_TOKEN
    mocked_access_manger = mocker.patch.object(dialog, "access_manager")
    dialog._post_auth_request_refresh_token()
    assert mocked_access_manger.post.call_count == 1

    args = mocked_access_manger.post.call_args[0]
    network, args = args

    assert isinstance(network, QNetworkRequest)
    url = network.url().url()
    assert "refresh_tokens" in url

    args = literal_eval(args.decode())
    assert len(args) == 3
    assert args.get("refresh_token") == "dummy_token"
