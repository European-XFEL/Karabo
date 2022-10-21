#############################################################################
# Login dialog that configures itself in different modes (user auth, access
# level or read-only) depending on the information sent by a GUI Server.
#
# Created on 13.09.2022.
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import getpass
import json
from enum import IntEnum, unique
from struct import calcsize, unpack

from qtpy import uic
from qtpy.QtCore import QSize, Qt, QTimer, QUrl, Slot
from qtpy.QtGui import QIntValidator
from qtpy.QtNetwork import (
    QNetworkAccessManager, QNetworkReply, QNetworkRequest, QSslConfiguration,
    QSslSocket, QTcpSocket)
from qtpy.QtWidgets import QComboBox, QDialog

from karabo.native import decodeBinary
from karabogui.util import get_spin_widget

from .utils import get_dialog_ui

DIALOG_HEIGHT_TOP_GROUP_BOX = 242
GROUP_BOX_HEIGHT = 116

DELAY_CONNECT_SERVER_MS = 500


@unique
class LoginType(IntEnum):
    USER_AUTHENTICATED = 0
    ACCESS_LEVEL = 1
    READ_ONLY = 2
    UNKNOWN = 3


class ReactiveLoginDialog(QDialog):
    def __init__(self, username="", access_level="admin", hostname="", port="",
                 gui_servers=[], parent=None):
        super(ReactiveLoginDialog, self).__init__(parent)
        filepath = get_dialog_ui("reactive_login_dialog.ui")
        uic.loadUi(filepath, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        self.login_type = LoginType.UNKNOWN
        self.getting_serv_info = False
        self.queried_host = ""
        self.queried_port = ""
        self.authenticating_user = False
        self.auth_url = ""
        self.auth_user = ""
        self.one_time_token = ""
        self.err_msg = ""
        self.topic = ""

        # Initialization of communication objects needed for authenticating
        # users.
        self.net_mngr = QNetworkAccessManager()
        self.net_mngr.finished.connect(self._handle_auth_reply)
        # TODO Remove the code below that disables SSL certification path
        #      verification as soon as real production SSL certificates are
        #      used.
        ssl = QSslConfiguration.defaultConfiguration()
        ssl.setPeerVerifyMode(QSslSocket.VerifyNone)
        QSslConfiguration.setDefaultConfiguration(ssl)

        # The socket used to get information from a GUI Server
        self.tcp_sock = QTcpSocket(self)
        self.tcp_sock.readyRead.connect(self._on_read_gui_serv_info)
        self.tcp_sock.error.connect(self._on_gui_serv_sock_error)

        self.spin_widget = get_spin_widget(icon="wait-black",
                                           scaled_size=QSize(16, 16))
        self.spin_widget.setVisible(False)
        self.h_layout_status.insertWidget(0, self.spin_widget)

        self.conn_serv_timer = QTimer()
        self.conn_serv_timer.setInterval(DELAY_CONNECT_SERVER_MS)
        self.conn_serv_timer.timeout.connect(self._on_conn_serv_timer_tick)

        hostname = hostname if hostname else "localhost"
        self.combo_serv_hostname.editTextChanged.connect(
            self._on_hostname_editText_changed)
        self.combo_serv_hostname.setInsertPolicy(QComboBox.NoInsert)
        self.combo_serv_hostname.setEditable(True)
        for server in gui_servers:
            self.combo_serv_hostname.addItem(server)
        self.combo_serv_hostname.setEditText(hostname)

        port = str(port) if port else "44444"
        self.edit_serv_port.setText(port)
        self.edit_serv_port.setValidator(QIntValidator(1, 65535))
        self.edit_serv_port.textEdited.connect(self._on_port_changed)

        self.edit_auth_username.textEdited.connect(self._on_username_changed)
        self.edit_auth_password.textEdited.connect(self._on_password_changed)

        # Use hard-coded list instead of AccessLevel enum to keep compatibility
        # with the existing dialog.
        for acc_lvl in ["admin", "expert", "operator", "user", "observer"]:
            self.combo_lev_access.addItem(acc_lvl)
        lvl_access = self.combo_lev_access.findText(access_level)
        self.combo_lev_access.setCurrentIndex(lvl_access)

        # For Authenticated login, initialize the user with any externally
        # supplied username.
        self.edit_auth_username.setText(username)

        # For Access Level and Read-Only logins the user of the current
        # shell is used.
        shell_user = getpass.getuser()
        self.label_lev_user_value.setText(shell_user)
        self.label_readonly_user_value.setText(shell_user)

        self.btn_connect.clicked.connect(self._on_connect_clicked)

        self.setFixedSize(self.size().width(),
                          DIALOG_HEIGHT_TOP_GROUP_BOX + GROUP_BOX_HEIGHT)

        self._update_dialog_state()

    # --------------------------------------------------------------------
    # Dialog Public Properties

    @property
    def access_level(self) -> str:
        al = "observer"
        if self.login_type == LoginType.ACCESS_LEVEL:
            al = self.combo_lev_access.currentText()
        return al

    @ property
    def username(self):
        user = "."
        if self.login_type == LoginType.USER_AUTHENTICATED:
            user = self.edit_auth_username.text()
        elif self.login_type == LoginType.ACCESS_LEVEL:
            user = self.label_lev_user_value.text()
        elif self.login_type == LoginType.READ_ONLY:
            user = self.label_readonly_user_value.text()
        return user

    @ property
    def hostname(self):
        return self.combo_serv_hostname.currentText()

    @ property
    def port(self):
        return int(self.edit_serv_port.text())

    @ property
    def gui_servers(self):
        return [self.combo_serv_hostname.itemText(i)
                for i in range(self.combo_serv_hostname.count())]

    # --------------------------------------------------------------------
    # Internal Slots

    @ Slot(str)
    def _on_hostname_editText_changed(self, value):
        """Split the selected text into hostname and port and trigger the
        connect to server timer.
        """
        if ":" in value:
            hostname, port = value.split(":")
            self.combo_serv_hostname.setEditText(hostname)
            if port.isnumeric():
                self.edit_serv_port.setText(port)
        self.err_msg = ""
        if (len(self.combo_serv_hostname.currentText().strip()) > 3 and
                len(self.edit_serv_port.text()) > 0):
            self.conn_serv_timer.start()
        else:
            self.conn_serv_timer.stop()
        self._update_dialog_state()

    @ Slot()
    def _on_port_changed(self):
        """Trigger the connect to server timer."""
        self.err_msg = ""
        if (len(self.combo_serv_hostname.currentText().strip()) > 3 and
                len(self.edit_serv_port.text().strip()) > 0):
            self.conn_serv_timer.start()
        else:
            self.conn_serv_timer.stop()
        self._update_dialog_state()

    @ Slot()
    def _on_conn_serv_timer_tick(self):
        self.conn_serv_timer.stop()
        self._connect_gui_server()

    @ Slot()
    def _on_username_changed(self):
        self._update_dialog_state()

    @ Slot()
    def _on_password_changed(self):
        self._update_dialog_state()

    @ Slot()
    def _on_connect_clicked(self):
        self.err_msg = ""
        if self.login_type == LoginType.USER_AUTHENTICATED:
            self._post_auth_request()
        else:
            self.accept()

    # --------------------------------------------------------------------
    # Server Info Retrieval

    def _connect_gui_server(self):
        self.queried_host = self.combo_serv_hostname.currentText().strip()
        self.queried_port = self.edit_serv_port.text().strip()
        self.getting_serv_info = True
        self.topic = ""
        self.login_type = LoginType.UNKNOWN
        self.tcp_sock.connectToHost(self.queried_host, int(self.queried_port))
        self._update_dialog_state()

    def _on_read_gui_serv_info(self):
        """Reads the Hash of type "brokerInformation" (or "serverInformation")
        sent by the connected GUI Server."""

        # Reads the size of the Hash
        bytesNeededSize = calcsize('I')
        rawBytesNeeded = self.tcp_sock.read(bytesNeededSize)
        bytesNeeded = unpack('I', rawBytesNeeded)[0]
        # Reads the Hash
        dataBytes = self.tcp_sock.read(bytesNeeded)
        # print(f"Hash Data ({len(dataBytes)} bytes):\n{dataBytes}")
        server_info = decodeBinary(dataBytes)

        if not self.getting_serv_info:
            # The connection info received is not for the latest attempt to
            # connect to a GUI Server - ignore it.
            self.tcp_sock.disconnectFromHost()
            self._update_dialog_state()
            return

        if server_info.has("readOnly") and server_info["readOnly"]:
            self.login_type = LoginType.READ_ONLY
        elif (server_info.has("authServer") and
              bool(server_info["authServer"].strip())):
            self.login_type = LoginType.USER_AUTHENTICATED
            self.edit_auth_password.setText("")
            self.edit_auth_username.setFocus()
            self.auth_url = server_info["authServer"].strip()
            if not self.auth_url.endswith("/"):
                self.auth_url += "/"
            self.auth_url += "auth_once_tk"
        else:
            self.login_type = LoginType.ACCESS_LEVEL
        self.topic = f"Topic: {server_info['topic']}"

        # print(f"serverInfo:\n{server_info}\n")
        self.tcp_sock.disconnectFromHost()
        self.getting_serv_info = False
        self._update_dialog_state()

    def _on_gui_serv_sock_error(self, err):
        # print(f"Error: {err}")
        self.getting_serv_info = False
        self.err_msg = (
            "No GUI Server available at "
            f"'{self.queried_host}:{self.queried_port}'")
        self._update_dialog_state()

    # --------------------------------------------------------------------
    # Authentication Request Posting

    def _post_auth_request(self):
        self.authenticating_user = True
        self._update_dialog_state()

        creds = {
            "user": self.edit_auth_username.text(),
            "passwd": self.edit_auth_password.text()}
        creds_json = json.dumps(creds)

        req = QNetworkRequest(QUrl(self.auth_url))
        req.setHeader(QNetworkRequest.ContentTypeHeader,
                      'application/json')

        self.net_mngr.post(req, bytearray(creds_json.encode("utf-8")))

    @Slot(QNetworkReply)
    def _handle_auth_reply(self, reply):
        self.authenticating_user = False
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
                self.err_msg = auth_result["error_msg"]
        else:
            self.err_msg = reply.errorString()
        self._update_dialog_state()

    # --------------------------------------------------------------------
    # Dialog State Updating

    def _update_dialog_state(self):

        # Topic of currently selected server
        self.label_serv_topic.setText(self.topic)
        self.label_serv_topic.setVisible(bool(self.topic.strip()))

        # Visibility of per login type group boxes
        self.setUpdatesEnabled(False)
        self.group_login_auth.setVisible(
            self.login_type == LoginType.USER_AUTHENTICATED)
        self.group_login_level.setVisible(
            self.login_type == LoginType.ACCESS_LEVEL)
        self.group_login_readonly.setVisible(
            self.login_type == LoginType.READ_ONLY)
        self.group_prompt.setVisible(
            self.login_type == LoginType.UNKNOWN)
        self.setUpdatesEnabled(True)

        # Status message and processing indicator
        status_color = "black"
        status_text = ""
        self.spin_widget.setVisible(
            self.authenticating_user or self.getting_serv_info)
        if self.authenticating_user:
            status_text = (
                f"<b>Authenticating</b> user <b>{self.auth_user}</b> ...")
        elif self.getting_serv_info:
            status_text = (
                "Getting <b>info</b> for Server "
                f"<b>{self.queried_host}:{self.queried_port}</b> ...")
        elif bool(self.err_msg.strip()):
            status_color = "red"
            status_text = self.err_msg
        self.label_status.setStyleSheet(f"QLabel {{color: {status_color};}}")
        self.label_status.setText(status_text)

        # Status of Connect button
        if (self.login_type == LoginType.ACCESS_LEVEL or
                self.login_type == LoginType.READ_ONLY):
            self.btn_connect.setEnabled(True)
        elif self.login_type == LoginType.UNKNOWN:
            self.btn_connect.setEnabled(False)
        elif self.login_type == LoginType.USER_AUTHENTICATED:
            self.btn_connect.setEnabled(
                bool(self.edit_auth_username.text().strip() and
                     self.edit_auth_password.text().strip()))
