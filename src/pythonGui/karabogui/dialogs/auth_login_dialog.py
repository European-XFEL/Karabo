#############################################################################
# Login dialog with user authentication.
#
# Created on 04.08.2022.
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json

from qtpy import uic
from qtpy.QtCore import QSize, Qt, QUrl, Slot
from qtpy.QtGui import QIntValidator
from qtpy.QtNetwork import (
    QNetworkAccessManager, QNetworkReply, QNetworkRequest, QSslConfiguration,
    QSslSocket)
from qtpy.QtWidgets import QComboBox, QDialog

from karabogui.singletons.api import get_config
from karabogui.util import get_spin_widget

from .utils import get_dialog_ui


class AuthLoginDialog(QDialog):
    def __init__(self, username="", hostname="", port="",
                 gui_servers=[], parent=None):
        super(AuthLoginDialog, self).__init__(parent)
        filepath = get_dialog_ui("auth_login_dialog.ui")
        uic.loadUi(filepath, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        self.net_mngr = QNetworkAccessManager()
        self.net_mngr.finished.connect(self._handle_auth_reply)

        # TODO Remove the code below that disables SSL certification path
        #      verification as soon as real production SSL certificates are
        #      used.
        ssl = QSslConfiguration.defaultConfiguration()
        ssl.setPeerVerifyMode(QSslSocket.VerifyNone)
        QSslConfiguration.setDefaultConfiguration(ssl)

        self.auth_url = get_config()["auth_server_base_url"]
        if not self.auth_url.endswith("/"):
            self.auth_url += "/"
        self.auth_url += "auth_once_tk"

        self.authenticating = False
        self.auth_issue = ""

        # token returned by a successful authentication
        self.one_time_token = ""

        self.spin_widget = get_spin_widget(icon="wait-black",
                                           scaled_size=QSize(16, 16))
        self.spin_widget.setVisible(False)
        self.bottom_h_layout.insertWidget(0, self.spin_widget)

        self.ui_username.setText(username)
        self.ui_username.textEdited.connect(self.on_credentials_port_changed)
        self.ui_password.setText("")
        self.ui_password.textEdited.connect(self.on_credentials_port_changed)

        hostname = hostname if hostname else "localhost"
        self.ui_hostname.editTextChanged.connect(self.on_hostname_changed)
        self.ui_hostname.setInsertPolicy(QComboBox.NoInsert)

        self.ui_hostname.setEditable(True)
        for server in gui_servers:
            self.ui_hostname.addItem(server)
        self.ui_hostname.setEditText(hostname)

        port = str(port) if port else "44444"
        self.ui_port.setText(port)
        self.ui_port.setValidator(QIntValidator(None))
        self.ui_port.textEdited.connect(self.on_credentials_port_changed)

        self.btn_connect.clicked.connect(self.on_connect_clicked)

        if username:
            # User name already filled; put focus on password field.
            self.ui_password.setFocus()

        self._update_dialog_state()

    @Slot()
    def on_credentials_port_changed(self):
        self.auth_issue = ""
        self._update_dialog_state()

    @Slot(str)
    def on_hostname_changed(self, value):
        """Split the selected text into hostname and port
        """
        if ":" in value:
            hostname, port = value.split(":")
            self.ui_hostname.setEditText(hostname)
            if port.isnumeric():
                self.ui_port.setText(port)
        self._update_dialog_state()

    @Slot()
    def on_connect_clicked(self):
        self.auth_issue = ""
        self._post_auth_request()

    @property
    def username(self):
        return self.ui_username.text()

    @property
    def password(self):
        return self.ui_password.text()

    @property
    def hostname(self):
        return self.ui_hostname.currentText()

    @property
    def port(self):
        return int(self.ui_port.text())

    @property
    def gui_servers(self):
        return [self.ui_hostname.itemText(i)
                for i in range(self.ui_hostname.count())]

    def _update_dialog_state(self):

        label_style = "QLabel {color: black;}"
        label_text = ""
        if self.authenticating:
            label_text = "Authenticating user..."
        elif self.auth_issue:
            label_style = "QLabel {color: red;}"
            label_text = f"{self.auth_issue}"
        self.lbl_status.setStyleSheet(label_style)
        self.lbl_status.setText(label_text)

        connect_enabled = not self.authenticating and bool(
            self.ui_username.text().strip() and
            self.ui_password.text().strip() and
            self.ui_hostname.currentText().strip() and
            self.ui_port.text().strip()
        )

        self.btn_connect.setEnabled(connect_enabled)

        self.spin_widget.setVisible(self.authenticating)

    def _post_auth_request(self):
        self.authenticating = True
        self._update_dialog_state()

        creds = {
            "user": self.ui_username.text(),
            "passwd": self.ui_password.text()}
        creds_json = json.dumps(creds)

        req = QNetworkRequest(QUrl(self.auth_url))
        req.setHeader(QNetworkRequest.ContentTypeHeader,
                      'application/json')

        self.net_mngr.post(req, bytearray(creds_json.encode("utf-8")))

    @Slot(QNetworkReply)
    def _handle_auth_reply(self, reply):
        self.authenticating = False
        self._update_dialog_state()

        er = reply.error()

        if er == QNetworkReply.NoError:
            bytes_string = reply.readAll()

            reply_body = str(bytes_string, "utf-8")
            auth_result = json.loads(reply_body)
            if auth_result.get("success"):
                self.one_time_token = auth_result["once_token"]
                self.accept()
                return
            else:
                # There was no error at the http level, but the validation
                # failed - most probably invalid credentials.
                self.auth_issue = auth_result["error_msg"]
        else:
            self.auth_issue = reply.errorString()
        self._update_dialog_state()
