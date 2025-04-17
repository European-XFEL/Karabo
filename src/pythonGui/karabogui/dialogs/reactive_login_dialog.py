##############################################################################
# Created on September 13, 2022
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################

import getpass
import json
from enum import IntEnum, unique
from struct import calcsize, unpack

from qtpy import uic
from qtpy.QtCore import Qt, QTimer, QUrl, Signal, Slot
from qtpy.QtGui import QDesktopServices, QIntValidator, QKeyEvent, QKeySequence
from qtpy.QtNetwork import (
    QAbstractSocket, QNetworkAccessManager, QNetworkReply, QNetworkRequest,
    QSslConfiguration, QSslSocket, QTcpSocket)
from qtpy.QtWidgets import (
    QApplication, QDialog, QDialogButtonBox, QHBoxLayout, QLineEdit,
    QSizePolicy, QWidget)

from karabo.native import decodeBinary
from karabogui import access as krb_access
from karabogui.const import CLIENT_HOST
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_config, get_network
from karabogui.util import SignalBlocker

from .utils import get_dialog_ui

TIMER_DELAY = 500  # ms
TOKEN_CHECK_TIME = 3000  # 3s
REQUEST_HEADER = "application/json"

USER_INFO = """<p>You are logged in as '<span style=" font-weight:600;">
{username}</span>'. Click 'Connect' to continue or</p><p>'Switch User' to
change the user.</p>"""

ACCESS_LEVEL_INFO = """<html><head/><body><p><span style=" font-size:10pt;
font-style:italic; color:#242424;">Currently logged in as <b>'{user}'</b>
with the access level <b>'{access_level}'.</b></span></p><p><span style="
font-size:10pt; font-style:italic; color:#242424;">Open Access Form to
generate the access code by authenticating the user.
</span></p></body></html>"""

NO_REFRESH_TOKEN_ERROR = "Refresh Token not found"


@unique
class LoginType(IntEnum):
    UNKNOWN = 0
    USER_AUTHENTICATED = 1
    ACCESS_LEVEL = 2
    READ_ONLY = 3
    REFRESH_TOKEN = 4


class Validator(QIntValidator):
    """A custom validator that allows only 1 or 6 digits """

    def validate(self, input, pos):
        """Support only number with single digit or 6 digits"""
        if input in ("+", "-"):
            return self.Invalid, input, pos
        if not (pos and input):
            # On deleting item from cell
            return self.Intermediate, input, pos
        if len(input) not in (1, 6):
            return self.Invalid, input, pos
        if pos not in (1, 6):
            return self.Invalid, input, pos
        return super().validate(input, pos)


class Cell(QLineEdit):
    onBackspacePressed = Signal()
    moveToNextCell = Signal(bool)
    onPasteNewCode = Signal()

    def keyPressEvent(self, event: QKeyEvent) -> None:
        if event.key() == Qt.Key_Backspace:
            self.onBackspacePressed.emit()
        elif event.key() == Qt.Key_Right:
            self.moveToNextCell.emit(True)
        elif event.key() == Qt.Key_Left:
            self.moveToNextCell.emit(False)
        elif event.matches(QKeySequence.Paste):
            self.onPasteNewCode.emit()
        return super().keyPressEvent(event)


class AccessCodeWidget(QWidget):
    """A widget to display each digit in the access code in a separate cell(
    QLineEdit)"""

    valueChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setObjectName("AccessCodeWidget")

        self.cells = []
        for i in range(6):
            object_name = f"Cell_{i}"
            cell = Cell(parent=self)
            cell.setObjectName(object_name)
            cell.setFixedWidth(30)
            cell.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
            cell.setAlignment(Qt.AlignCenter)
            cell.setValidator(Validator(parent=self))
            self.cells.append(cell)
            layout.addWidget(cell)
            cell.textChanged.connect(self.on_text_changed)
            cell.onBackspacePressed.connect(self.on_backspace_pressed)
            cell.moveToNextCell.connect(self._on_move_to_cell)
            cell.onPasteNewCode.connect(self._paste_new_code)

    @Slot(str)
    def on_text_changed(self, text: str):
        current_index = self.cells.index(self.sender())
        # Distribute each digits in each cells.
        if len(text) == 6:
            self._paste_on_all_cell(text)
        # Paste one digit to current cell and move focus to the next.
        elif current_index < 5 and self.sender().text():
            next_index = current_index + 1
            self.cells[next_index].setFocus()
        self.valueChanged.emit()

    @Slot()
    def on_backspace_pressed(self) -> None:
        """Set focus on the previous cell when the cell is emtpy or the
        cursor is left to the digit."""
        sender = self.sender()
        index = self.cells.index(sender)
        if index > 0 and not (sender.text() and sender.cursorPosition()):
            index -= 1
        self.cells[index].setFocus()

    @Slot(bool)
    def _on_move_to_cell(self, forward: bool) -> None:
        """Move the cursor to the next/previous cell"""
        index = self.cells.index(self.sender())
        if forward and index != 5:
            index += 1
        if not forward and index != 0:
            index -= 1
        self.cells[index].setFocus()

    @Slot()
    def _paste_new_code(self):
        text = QApplication.clipboard().text()
        if len(text) == 6 and text.isdigit():
            self._paste_on_all_cell(text)
            self.valueChanged.emit()

    def get_access_code(self) -> str:
        return "".join(cell.text() for cell in self.cells)

    def has_access_code(self):
        return all([cell.text() for cell in self.cells])

    def _paste_on_all_cell(self, text):
        with SignalBlocker(self):
            for number, cell in zip(str(text), self.cells):
                cell.setText(number)


BUTTON_STYLE = """
QPushButton#skip_authentication_button{border: none; color: rgb(80,133,207)}
QPushButton#skip_authentication_button:hover{border: none;
                                             color: rgb(8,8,245)}"""


class ReactiveLoginDialog(QDialog):
    def __init__(self, access_level="expert", hostname="", port="",
                 gui_servers=None, parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui("reactive_login_dialog.ui")
        uic.loadUi(filepath, self)
        if parent is None:
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

        self._token_check_timer = QTimer(parent=self)
        self._token_check_timer.timeout.connect(self._look_for_token)

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

        for level in ["expert", "operator", "observer"]:
            self.combo_access_level.addItem(level)
        access = self.combo_access_level.findText(access_level)
        self.combo_access_level.setCurrentIndex(access)

        # Access Level and Read-Only login
        shell_user = getpass.getuser()
        self.label_shell_user.setText(shell_user)
        self.label_readonly_user.setText(shell_user)

        self.edit_access_code = AccessCodeWidget(parent)
        self.access_widget_layout.addWidget(self.edit_access_code)
        self.edit_access_code.valueChanged.connect(self._update_button)

        self.authenticate_button.clicked.connect(self.open_login_webpage)

        self.connect_button.clicked.connect(self.connect_clicked)
        self._update_dialog_state()

        if self.hasAcceptableInput():
            self._timer.start()

        self.switch_button.clicked.connect(self._switch_to_auth_page)
        self.skip_authentication_button.setStyleSheet(BUTTON_STYLE)
        self.skip_authentication_button.clicked.connect(self.accept)

    # --------------------------------------------------------------------
    # Dialog Public Properties

    @property
    def access_level(self) -> str:
        level = get_config()["access_level"]
        if self.login_type is LoginType.ACCESS_LEVEL:
            level = self.combo_access_level.currentText()
        return level

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
        self._update_button()

    @Slot()
    def connect_clicked(self):
        self._error = None
        if self.login_type is LoginType.USER_AUTHENTICATED:
            self._login_authenticated()
        elif self.login_type is LoginType.REFRESH_TOKEN:
            self._refresh_authentication()
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
            if get_config()["refresh_token"] is not None:
                self.login_type = LoginType.REFRESH_TOKEN
            self._auth_url = server_info["authServer"]
            if not self._auth_url.endswith("/"):
                self._auth_url += "/"
            krb_access.AUTHENTICATION_SERVER = self._auth_url
        else:
            self.login_type = LoginType.ACCESS_LEVEL

        if self.login_type in (LoginType.USER_AUTHENTICATED,
                               LoginType.REFRESH_TOKEN):
            self._token_check_timer.start(TOKEN_CHECK_TIME)
        elif self._token_check_timer.isActive():
            self._token_check_timer.stop()

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
        if self._token_check_timer.isActive():
            self._token_check_timer.stop()

    @Slot(QNetworkReply)
    def onAuthReply(self, reply):
        self._authenticating = False
        self._update_dialog_state()

        url = reply.url().path()
        if url.endswith("user_tokens") and not self.remember_login.isChecked():
            del get_config()["refresh_token"]
            del get_config()["refresh_token_user"]

        error = reply.error()
        bytes_string = reply.readAll()
        reply_body = str(bytes_string, "utf-8")
        auth_result = json.loads(reply_body)
        if error == QNetworkReply.NoError and auth_result["success"]:
            krb_access.ONE_TIME_TOKEN = auth_result["once_token"]
            refresh_token = auth_result.get("refresh_token")
            if refresh_token is not None:
                refresh_token_user = auth_result.get("username")
                get_config()["refresh_token"] = refresh_token
                get_config()["refresh_token_user"] = refresh_token_user
            self.accept()
        else:
            self._error = auth_result.get("error_msg")
            if self._error == NO_REFRESH_TOKEN_ERROR:
                del get_config()["refresh_token"]
                del get_config()["refresh_token_user"]
            self.stackedWidget.setCurrentIndex(LoginType.USER_AUTHENTICATED)
            self._update_status_label()

        reply.deleteLater()

    @Slot()
    def open_login_webpage(self):
        """Open browser to load the login page."""
        url = f"{self._auth_url}login_form"
        success = QDesktopServices.openUrl(QUrl(url))
        if not success:
            self._error = f"Failed to open the page {url}."
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

        if self.login_type is LoginType.REFRESH_TOKEN:
            text = USER_INFO.format(
                username=get_config()["refresh_token_user"])
            self.user_info_label.setText(text)

    def _update_button(self):
        """Update the connect button according to the status"""
        if self.login_type in (LoginType.ACCESS_LEVEL, LoginType.READ_ONLY):
            self.connect_button.setEnabled(True)
        elif self.login_type is LoginType.UNKNOWN:
            self.connect_button.setEnabled(False)
        elif self.login_type is LoginType.USER_AUTHENTICATED:
            enable = self.edit_access_code.has_access_code()
            self.connect_button.setEnabled(enable)
        elif self.login_type is LoginType.REFRESH_TOKEN:
            self.connect_button.setEnabled(True)

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

    def _login_authenticated(self):
        """Authentication Request Posting"""
        self._authenticating = True
        self._update_dialog_state()

        remember_login = self.remember_login.isChecked()
        info = json.dumps({
            "access_code": int(self.edit_access_code.get_access_code()),
            "client_hostname": CLIENT_HOST,
            "remember_login": remember_login
        })
        if not remember_login:
            # Erase existing information
            del get_config()["refresh_token"]
            del get_config()["refresh_token_user"]

        info = bytearray(info.encode("utf-8"))
        url = f"{self._auth_url}user_tokens"

        request = QNetworkRequest(QUrl(url))
        request.setHeader(
            QNetworkRequest.ContentTypeHeader, REQUEST_HEADER)
        self.access_manager.post(request, info)

    def _refresh_authentication(self):
        self._authenticating = True
        self._update_dialog_state()

        info = json.dumps({
            "refresh_token": get_config()["refresh_token"],
            "username": get_config()["refresh_token_user"],
            "client_hostname": CLIENT_HOST,
        })
        info = bytearray(info.encode("utf-8"))
        url = f"{self._auth_url}refresh_tokens"

        request = QNetworkRequest(QUrl(url))
        request.setHeader(QNetworkRequest.ContentTypeHeader, REQUEST_HEADER)
        self.access_manager.post(request, info)

    @Slot()
    def _switch_to_auth_page(self):
        self.login_type = LoginType.USER_AUTHENTICATED
        self._update_dialog_state()
        if self._token_check_timer.isActive():
            self._token_check_timer.stop()

    @Slot()
    def _look_for_token(self):
        if get_config()["refresh_token"] is None:
            self.login_type = LoginType.USER_AUTHENTICATED
        else:
            self.login_type = LoginType.REFRESH_TOKEN
        self._update_dialog_state()


TEMPORARY_INDEX = 0
PERMANENT_INDEX = 1


class UserSessionDialog(QDialog):
    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self.setModal(False)
        filepath = get_dialog_ui("temp_session_dialog.ui")
        uic.loadUi(filepath, self)
        if parent is None:
            self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        self.remember_login.setVisible(False)
        self.ok_button = self.buttonBox.button(QDialogButtonBox.Ok)
        self.ok_button.setEnabled(False)
        self.combo_mode.currentIndexChanged.connect(self._switch_temporary)
        if krb_access.is_end_of_session():
            self.combo_mode.setCurrentIndex(PERMANENT_INDEX)
            self.combo_mode.setEnabled(False)

        self.edit_access_code = AccessCodeWidget(parent)
        self.access_widget_layout.addWidget(self.edit_access_code)
        self.edit_access_code.valueChanged.connect(self._update_button)
        self.authenticate_button.clicked.connect(self.open_login_webpage)

        self.access_manager = QNetworkAccessManager()
        self.access_manager.finished.connect(self.onAuthReply)
        self._auth_url = krb_access.AUTHENTICATION_SERVER

        user = get_config()["username"]
        access_level = krb_access.GLOBAL_ACCESS_LEVEL.name
        text = ACCESS_LEVEL_INFO.format(user=user, access_level=access_level)
        self.info_label.setText(text)

        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.UserSession: self._event_user_session

        }
        register_for_broadcasts(self.event_map)

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def _event_user_session(self, data):
        self.close()

    @Slot(int)
    def _switch_temporary(self, index):
        self.remember_login.setVisible(bool(index))

    @Slot()
    def open_login_webpage(self):
        """Open browser to load the login page."""
        url = f"{self._auth_url}login_form"
        success = QDesktopServices.openUrl(QUrl(url))
        if not success:
            self.error_label.setText(f"Failed to open the page {url}.")

    @Slot()
    def accept(self):
        # We are calling the super method only after the authentication of
        # the temporary session user.
        self._login_authenticated()

    @Slot()
    def _update_button(self):
        enable = self.edit_access_code.has_access_code()
        self.ok_button.setEnabled(enable)

    def _login_authenticated(self):
        if self.combo_mode.currentIndex() == TEMPORARY_INDEX:
            remember_login = False
        else:
            remember_login = self.remember_login.isChecked()

        info = json.dumps({
            "access_code": int(self.edit_access_code.get_access_code()),
            "client_hostname": CLIENT_HOST,
            "remember_login": remember_login,
        })
        info = bytearray(info.encode("utf-8"))
        url = f"{self._auth_url}user_tokens"

        request = QNetworkRequest(QUrl(url))
        request.setHeader(QNetworkRequest.ContentTypeHeader, REQUEST_HEADER)
        self.access_manager.post(request, info)

    @Slot(QNetworkReply)
    def onAuthReply(self, reply: QNetworkReply):
        error = reply.error()
        bytes_string = reply.readAll()
        reply_body = str(bytes_string, "utf-8")
        auth_result = json.loads(reply_body)
        if error == QNetworkReply.NoError and auth_result["success"]:
            krb_access.ONE_TIME_TOKEN = auth_result["once_token"]
            # Temporary Session
            if self.combo_mode.currentIndex() == TEMPORARY_INDEX:
                highest_access_level = krb_access.HIGHEST_ACCESS_LEVEL.name
                krb_access_level = krb_access.ACCESS_LEVEL_MAP.get(
                    highest_access_level.lower(), 2)
                username = get_config()["username"]
                get_network().beginTemporarySession(
                    username=username,
                    temporarySessionToken=auth_result["once_token"],
                    levelBeforeTemporarySession=krb_access_level,
                )
            else:
                refresh_token = auth_result.get("refresh_token")
                if refresh_token is not None:
                    refresh_token_user = auth_result.get("username")
                    get_config()["refresh_token"] = refresh_token
                    get_config()["refresh_token_user"] = refresh_token_user
                else:
                    # In case we haven't received a refresh token, we are not
                    # remembering and thus have to erase tokens.
                    del get_config()["refresh_token"]
                    del get_config()["refresh_token_user"]
                get_network().onLogin()

            super().accept()
        else:
            error = auth_result.get("error_msg")
            self.error_label.setStyleSheet(
                "QLabel#error_label {color:red;}")
            self.error_label.setText(error)
        reply.deleteLater()
