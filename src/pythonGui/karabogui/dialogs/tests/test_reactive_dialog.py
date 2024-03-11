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
