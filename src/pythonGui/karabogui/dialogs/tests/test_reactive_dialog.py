from karabogui import access as krb_access
from karabogui.dialogs.reactive_login_dialog import (
    LoginType, ReactiveLoginDialog)


def test_access_level(gui_app):
    dialog = ReactiveLoginDialog()
    assert dialog.access_level == krb_access.GLOBAL_ACCESS_LEVEL.name.lower()

    dialog.login_type = LoginType.ACCESS_LEVEL
    assert dialog.access_level == "admin"
