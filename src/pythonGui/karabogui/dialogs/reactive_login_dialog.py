##############################################################################
# Created on September 13, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import getpass
import json
from enum import IntEnum, unique
from struct import calcsize, unpack

from qtpy import uic
from qtpy.QtCore import Qt, QTimer, QUrl, Slot
from qtpy.QtGui import QIntValidator
from qtpy.QtNetwork import (
    QAbstractSocket, QNetworkAccessManager, QNetworkReply, QNetworkRequest,
    QSslConfiguration, QSslSocket, QTcpSocket)
from qtpy.QtWidgets import QDialog

from karabo.native import decodeBinary

from .utils import get_dialog_ui

TIMER_DELAY = 500  # ms
REQUEST_HEADER = "application/json"


@unique
class LoginType(IntEnum):
    UNKNOWN = 0
    USER_AUTHENTICATED = 1
    ACCESS_LEVEL = 2
    READ_ONLY = 3


class ReactiveLoginDialog(QDialog):
    def __init__(self, username="", access_level="admin", hostname="", port="",
                 gui_servers=None, parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui("reactive_login_dialog.ui")
        uic.loadUi(filepath, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        self.stackedWidget.setCurrentIndex(0)
        self.login_type = LoginType.UNKNOWN
        self._requesting = False
        self._queried_host = ""
        self._queried_port = ""
        self._authenticating = False
        self._auth_url = ""
        self._auth_user = ""
        self._error = None
        self._one_time_token = ""
        self._topic = ""

        # Initialization of communication objects needed for authenticating
        # users.
        self.access_manager = QNetworkAccessManager()
        self.access_manager.finished.connect(self.onAuthReply)
        # XXX: Remove the code below that disables SSL certification path
        # verification as soon as real production SSL certificates are
        # used.
        ssl = QSslConfiguration.defaultConfiguration()
        ssl.setPeerVerifyMode(QSslSocket.VerifyNone)
        QSslConfiguration.setDefaultConfiguration(ssl)

        # The socket used to get information from a GUI Server
        self._tcp_socket = QTcpSocket(self)
        self._tcp_socket.readyRead.connect(self.onReadServerData)
        self._tcp_socket.error.connect(self.onSocketError)

        self._timer = QTimer(self)
        self._timer.setInterval(TIMER_DELAY)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self.connectToServer)

        hostname = hostname if hostname else "localhost"
        if gui_servers is None:
            gui_servers = []
        for server in gui_servers:
            self.combo_hostname.addItem(server)
        self.combo_hostname.setEditText(hostname)
        self.combo_hostname.editTextChanged.connect(self.on_hostname_changed)

        port = str(port) if port else "44444"
        self.edit_port.setText(port)
        self.edit_port.setValidator(QIntValidator(1, 65535, parent=self))
        self.edit_port.textEdited.connect(self.on_port_changed)

        for level in ["admin", "expert", "operator", "user", "observer"]:
            self.combo_access_level.addItem(level)
        access = self.combo_access_level.findText(access_level)
        self.combo_access_level.setCurrentIndex(access)

        # Authenticated login
        self.edit_username.setText(username)
        self.edit_username.textEdited.connect(self._on_username_changed)
        self.edit_password.textEdited.connect(self._on_password_changed)

        # Access Level and Read-Only login
        shell_user = getpass.getuser()
        self.label_shell_user.setText(shell_user)
        self.label_readonly_user.setText(shell_user)

        self.connect_button.clicked.connect(self.connect_clicked)
        self._update_dialog_state()

        if self.hasAcceptableInput():
            self._timer.start()

    # --------------------------------------------------------------------
    # Dialog Public Properties

    @property
    def one_time_token(self):
        """The network one time token used to authenticate"""
        return self._one_time_token

    @property
    def access_level(self) -> str:
        level = "observer"
        if self.login_type is LoginType.ACCESS_LEVEL:
            level = self.combo_access_level.currentText()
        return level

    @property
    def username(self):
        user = "."
        if self.login_type is LoginType.USER_AUTHENTICATED:
            user = self.edit_username.text()
        elif self.login_type is LoginType.ACCESS_LEVEL:
            user = self.label_shell_user.text()
        elif self.login_type is LoginType.READ_ONLY:
            user = self.label_readonly_user.text()
        return user

    @property
    def hostname(self):
        return self.combo_hostname.currentText()

    @property
    def port(self):
        return int(self.edit_port.text())

    @property
    def gui_servers(self):
        return [self.combo_hostname.itemText(i)
                for i in range(self.combo_hostname.count())]

    def hasAcceptableInput(self):
        """Return if the dialog has acceptable input for the host and port"""
        host = len(self.combo_hostname.currentText()) > 3
        port = len(self.edit_port.text()) > 0
        return host and port

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot(str)
    def on_hostname_changed(self, value):
        """Split the selected text into hostname and port and trigger the
        connect to server timer.
        """
        if ":" in value:
            hostname, port = value.split(":")
            self.combo_hostname.setEditText(hostname)
            if port.isnumeric():
                self.edit_port.setText(port)

        if self.hasAcceptableInput():
            self._timer.start()

    @Slot()
    def on_port_changed(self):
        if self.hasAcceptableInput():
            self._timer.start()

    @Slot()
    def _on_username_changed(self):
        self._update_dialog_state()

    @Slot()
    def _on_password_changed(self):
        self._update_dialog_state()

    @Slot()
    def connect_clicked(self):
        self._error = None
        if self.login_type is LoginType.USER_AUTHENTICATED:
            self._post_auth_request()
        else:
            self.accept()

    @Slot()
    def connectToServer(self):
        """Connect to the server"""
        if self._tcp_socket.state() != QTcpSocket.UnconnectedState:
            self._tcp_socket.abort()

        self._queried_port = self.edit_port.text()
        if not self._queried_port:
            return
        self._requesting = True
        self._queried_host = self.combo_hostname.currentText()
        self._topic = ""
        self._error = None
        self.login_type = LoginType.UNKNOWN
        self._tcp_socket.connectToHost(self._queried_host,
                                       int(self._queried_port))
        self._update_dialog_state()

    @Slot()
    def onReadServerData(self):
        """Read data coming via the tcp socket"""
        bytesNeededSize = calcsize("I")
        rawBytesNeeded = self._tcp_socket.read(bytesNeededSize)
        bytesNeeded = unpack("I", rawBytesNeeded)[0]

        dataBytes = self._tcp_socket.read(bytesNeeded)
        server_info = decodeBinary(dataBytes)

        if not self._requesting:
            # The connection info received is not for the latest attempt to
            # connect to a GUI Server - ignore it.
            self._tcp_socket.disconnectFromHost()
            self._update_dialog_state()
            return

        if server_info.has("readOnly") and server_info["readOnly"]:
            self.login_type = LoginType.READ_ONLY
        elif (server_info.has("authServer") and
              bool(server_info["authServer"])):
            self.login_type = LoginType.USER_AUTHENTICATED
            self.edit_password.setText("")
            self.edit_username.setFocus()
            self._auth_url = server_info["authServer"]
            if not self._auth_url.endswith("/"):
                self._auth_url += "/"
            self._auth_url += "auth_once_tk"
        else:
            self.login_type = LoginType.ACCESS_LEVEL

        self._topic = f"Topic: {server_info['topic']}"
        self._tcp_socket.disconnectFromHost()
        self._requesting = False
        self._update_dialog_state()

    @Slot(QAbstractSocket.SocketError)
    def onSocketError(self, error):
        self._requesting = False
        self._error = ("No GUI Server available at "
                       f"{self._queried_host}:{self._queried_port}")
        self._update_dialog_state()

    @Slot(QNetworkReply)
    def onAuthReply(self, reply):
        self._authenticating = False
        # XXX: Label sufficient?
        self._update_dialog_state()

        error = reply.error()

        if error == QNetworkReply.NoError:
            bytes_string = reply.readAll()
            reply_body = str(bytes_string, "utf-8")
            auth_result = json.loads(reply_body)
            if auth_result.get("success"):
                self._one_time_token = auth_result["once_token"]
                self.accept()
                return
            else:
                # There was no error at the http level, but the validation
                # failed - most probably invalid credentials.
                self._error = auth_result["error_msg"]
        else:
            self._error = reply.errorString()

        self._update_status_label()

    # --------------------------------------------------------------------
    # Private interface

    def _update_dialog_state(self):
        """Update the stacked widget of the dialog and the status label"""
        self.label_topic.setText(self._topic)
        self.label_topic.setVisible(bool(self._topic))
        self.stackedWidget.setCurrentIndex(self.login_type.value)

        self._update_status_label()
        # Status of Connect button
        self._update_button()

    def _update_button(self):
        """Update the connect button according to the status"""
        if self.login_type in (LoginType.ACCESS_LEVEL, LoginType.READ_ONLY):
            self.connect_button.setEnabled(True)
        elif self.login_type is LoginType.UNKNOWN:
            self.connect_button.setEnabled(False)
        elif self.login_type is LoginType.USER_AUTHENTICATED:
            username = self.edit_username.text()
            password = self.edit_password.text()
            self.connect_button.setEnabled(bool(username and password))

    def _update_status_label(self):
        """Status message and processing indicator"""
        status_color = "black"
        status_text = ""
        if self._authenticating:
            status_text = (
                f"<b>Authenticating</b> user <b>{self._auth_user}</b> ...")
        elif self._requesting:
            status_text = (
                "Getting <b>info</b> for Server "
                f"<b>{self._queried_host}:{self._queried_port}</b> ...")
        elif self._error is not None:
            status_color = "red"
            status_text = self._error
        self.label_status.setStyleSheet(f"QLabel {{color: {status_color};}}")
        self.label_status.setText(status_text)

    def _post_auth_request(self):
        """Authentication Request Posting"""
        self._authenticating = True
        self._update_dialog_state()

        credentials = json.dumps({
            "user": self.edit_username.text(),
            "passwd": self.edit_password.text(),
        })
        credentials = bytearray(credentials.encode("utf-8"))
        request = QNetworkRequest(QUrl(self._auth_url))
        request.setHeader(QNetworkRequest.ContentTypeHeader,
                          REQUEST_HEADER)

        self.access_manager.post(request, credentials)
