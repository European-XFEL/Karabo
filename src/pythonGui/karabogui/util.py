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
import html
import os
import os.path as op
import re
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path
from tempfile import mkstemp
from typing import Any
from uuid import uuid4
from xml.sax.saxutils import escape

import numpy as np
from dateutil.tz import tzlocal, tzutc
from qtpy import QtCore
from qtpy.QtCore import QEvent, QEventLoop, QObject, QSize, Qt
from qtpy.QtGui import QCursor, QIcon, QMovie, QValidator
from qtpy.QtWidgets import (
    QApplication, QDialog, QFileDialog, QLabel, QMessageBox)

from karabo.native import Hash, decodeXML, writeXML
from karabogui import const, icons, messagebox
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, extract_configuration)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_config, get_db_conn, get_network
from karabogui.singletons.util import get_error_message


def send_info(**info: Any):
    """A convenience wrapper to send an info to the guiserver"""
    assert "type" in info, "Need to specify a `type`"
    network = get_network()
    network.onInfo(Hash(info))


def get_application_icon():
    logo_path = str(Path(__file__).parent / "icons" / "app_logo.png")
    return QIcon(logo_path)


class MouseWheelEventBlocker(QObject):
    """A QObject which can be used for event filtering of mouse wheel events.
    """

    def __init__(self, widget):
        super().__init__()
        self.widget = widget

    def eventFilter(self, obj, event):
        # Block wheel events
        return event.type() == QEvent.Wheel and obj is self.widget


class SignalBlocker:
    """Block signals from QWidgets in a with statement"""

    def __init__(self, *objects):
        self.objects = objects

    def __enter__(self):
        self.states = [obj.blockSignals(True) for obj in self.objects]

    def __exit__(self, a, b, c):
        for obj, state in zip(self.objects, self.states):
            obj.blockSignals(state)


def generateObjectName(widget):
    return f"{widget.__class__.__name__}_{uuid4().hex}"


def getOpenFileName(parent=None, caption="", filter="", directory=""):
    """ Return `filename` of the Qt file open dialog.
    """
    directory = directory or const.HIDDEN_KARABO_FOLDER

    filename, _ = QFileDialog.getOpenFileName(
        parent=parent,
        caption=caption,
        directory=directory,
        filter=filter,
        options=QFileDialog.DontUseNativeDialog)

    return filename


def getSaveFileName(parent=None, caption="", filter="", directory="",
                    suffix="", selectFile=""):
    directory = directory or const.HIDDEN_KARABO_FOLDER

    dialog = QFileDialog(parent, caption, directory, filter)
    dialog.selectFile(selectFile)
    dialog.setDefaultSuffix(suffix)
    dialog.setFileMode(QFileDialog.AnyFile)
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    if filter:
        dialog.setNameFilter(filter)

    if dialog.exec() == QDialog.Rejected:
        return
    if len(dialog.selectedFiles()) == 1:
        return dialog.selectedFiles()[0]


def load_configuration_from_file(device_proxy, parent=None):
    """Given a ``BaseDeviceProxy`` object instance. Read a configuration Hash
    from an XML file and assign it to the object.
    """
    if device_proxy is None or device_proxy.binding is None:
        msg = ('Selected {} cannot load a configuration at this time because '
               'it does not have a schema.')
        is_class = isinstance(device_proxy, DeviceClassProxy)
        messagebox.show_error(msg.format('class' if is_class else 'device'))
        return
    config = get_config()
    path = config['data_dir']
    directory = path if path and op.isdir(path) else ""

    filename = getOpenFileName(caption="Open configuration",
                               filter="XML (*.xml)",
                               directory=directory,
                               parent=parent)
    if not filename:
        return

    with open(filename, 'rb') as fp:
        config = decodeXML(fp.read())

    # Save the directory information
    get_config()['data_dir'] = op.dirname(filename)
    # Broadcast so the configurator can handle the complexities of applying
    # a configuration.
    broadcast_event(KaraboEvent.LoadConfiguration,
                    {'proxy': device_proxy, 'configuration': config})


def save_configuration_to_file(device_proxy, parent=None):
    """This function saves the current configuration of a device to a file.
    """
    if device_proxy is None or device_proxy.binding is None:
        messagebox.show_error("No configuration available. Saving failed.")
        return
    config = get_config()
    path = config['data_dir']
    directory = path if path and op.isdir(path) else ""

    class_id = device_proxy.binding.class_id

    # use deviceId for existing device proxies
    if isinstance(device_proxy, DeviceProxy):
        default_name = device_proxy.device_id.replace('/', '-') + '.xml'
    else:
        default_name = class_id + '.xml'

    filename = getSaveFileName(caption="Save configuration as",
                               filter="Configuration (*.xml)",
                               suffix="xml",
                               directory=directory,
                               selectFile=default_name,
                               parent=parent)
    if not filename:
        return

    # If we are shutdown in between, either server or device, notify!
    if device_proxy.binding is None or not len(device_proxy.binding.value):
        messagebox.show_error("The configuration is empty and cannot "
                              "be saved", title='No configuration',
                              parent=parent)
        return

    config = Hash(class_id, extract_configuration(device_proxy.binding))

    # Save configuration to file
    with open(filename, 'w') as fp:
        writeXML(config, fp)

    # save the last config directory
    get_config()['data_dir'] = op.dirname(filename)


@contextmanager
def temp_file(suffix='', prefix='tmp', dir=None):
    """ Create a temporary file wrapped in a context manager.
        Usage is straightforward:
        with temp_file() as path:
            # Write a file to path

        # All traces of path are now gone
    """
    fd, filename = mkstemp(suffix=suffix, prefix=prefix, dir=dir)
    try:
        yield filename
    finally:
        os.close(fd)
        os.unlink(filename)


@contextmanager
def wait_cursor():
    """Simple context manager to make busy operations"""
    try:
        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
        yield
    finally:
        QApplication.restoreOverrideCursor()


def show_wait_cursor(func):
    """Show the processing ``busy`` function in the karabo gui"""

    def wrapper(*args, **kwargs):
        try:
            QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
            return func(*args, **kwargs)
        finally:
            QApplication.restoreOverrideCursor()

    return wrapper


def is_database_processing():
    """Return whether database is currently processing something. A message box
    is shown if the processing is active.
    """
    # Make sure there are not pending DB things in the pipe
    if get_db_conn().is_processing():
        msg = ('There is currently data fetched from or sent to the <br>'
               '<b>project database</b>. Please wait until this is done!')
        messagebox.show_warning(msg, 'Database connection active')
        return True
    return False


def get_spin_widget(*, icon, scaled_size=QSize(), parent=None):
    """Return a ``QLabel`` containing a spinning icon.
    """
    spin_widget = QLabel(parent)
    movie = QMovie(op.join(op.abspath(op.dirname(icons.__file__)), icon))
    movie.setScaledSize(scaled_size)
    spin_widget.setMovie(movie)
    movie.start()
    return spin_widget


def utc_to_local(utc_str: str, format='%Y-%m-%d %H:%M:%S') -> str:
    """Convert given `utc_str` in a given `format` to the local time string
    """
    if not utc_str:
        return ''

    utc_ts = datetime.strptime(utc_str, format)
    local_ts = utc_ts.replace(tzinfo=tzutc()).astimezone(tzlocal())
    return datetime.strftime(local_ts, format)


def utc_to_local_date(utc_str: str, format='%Y-%m-%d %H:%M:%S') -> datetime:
    """Convert given `utc_str` in a given `format` to the local datetime
    """
    if not utc_str:
        dt = datetime.now(tz=tzutc())
    else:
        dt = datetime.strptime(utc_str, format).replace(
            tzinfo=tzutc())
    # Convert to local naive datetime
    return dt.astimezone(tzlocal())


VALID_PROJECT_OBJECT_NAME = re.compile(r"^[\w-]+(/[\w-]+)*$")
# The macro name should start with a letter and can contain:
# letters, numbers, underscores and slashes
VALID_PROJECT_MACRO_NAME = re.compile(r"^[a-zA-Z][a-zA-Z0-9_\/]*$")


class InputValidator(QValidator):
    """ This class provides validation of user inputs within projects, whether
        it is about deviceIds, macro names, or scene names
    """

    def __init__(self, object_type=None, parent=None):
        super().__init__(parent)
        if object_type == "macro":
            self.pattern = VALID_PROJECT_MACRO_NAME
        else:
            self.pattern = VALID_PROJECT_OBJECT_NAME

    def validate(self, input, pos):
        if not input or input.endswith('/'):
            return self.Intermediate, input, pos

        if not self.pattern.match(input):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


def show_filename_error(filename, parent=None):
    invalid_chars = sorted(_get_invalid_chars(filename))

    # Make the space verbose
    if ' ' in invalid_chars:
        invalid_chars.remove(' ')
        invalid_chars.append("<space>")

    message = (
        "Filename contains the following invalid character(s):\n{}".format(
            ' '.join(invalid_chars)))

    messagebox.show_error(message, "Unable to Load File", parent=parent)


def _get_invalid_chars(filename):
    invalid_chars = []
    if not VALID_PROJECT_OBJECT_NAME.match(filename):
        invalid_regex = re.compile(r"[^a-zA-Z0-9\/\_\-]")
        invalid_chars = list(set(invalid_regex.findall(filename)))
    return invalid_chars


def move_to_cursor(widget):
    """Move the widget `widget` to the cursor

    This method quickly adjusts the size of the widget and then calculates
    an ideal position.
    """
    process_qt_events()

    pos = QCursor.pos()
    size = widget.size()

    x = pos.x() - size.width() // 2
    if x > 0:
        pos.setX(x)
    y = pos.y() - size.height() // 2
    if y > 0:
        pos.setY(y)
    widget.move(pos)


def process_qt_events(app=None, timeout=100):
    """Process the qt events of the application

    :param timeout: time in milliseconds, defaults to 100
    """
    if app is None:
        app = QApplication

    flags = QEventLoop.AllEvents | QEventLoop.WaitForMoreEvents
    app.processEvents(flags, timeout)


def create_table_string(info):
    """This creates a html table string from an `info` dictionary"""
    rows = (
         f"<tr><td><b>{attr}</b>:   </td><td>{escape(str(value))}</td></tr>"
         for attr, value in info.items()
    )
    return f"<table>{''.join(rows)}</table>"


def create_list_string(info):
    """Create a html string presentation from an `info` list"""
    sequence = "".join([f"<li>{arg}</li>" for arg in info])
    return f"<ul>{sequence}</ul>"


def version_compatible(version: str, major: int, minor: int):
    """Check if a karaboVersion string `version` fullfills the requirement

    The `major` and `minor` are the minimum requirements.
    """
    try:
        version_split = version.split(".")
        if len(version_split) == 1:
            # Note: this does not happen on the field, we set to `True`!
            return True
        if len(version_split) > 1:
            actual_major = int(version_split[0])
            actual_minor = int(version_split[1])
            if actual_major > major:
                return True
            elif actual_major == major:
                return actual_minor >= minor
        return False
    except Exception as e:
        print(f"Parsing karaboVersion string failed: {e}")
        return False


def qtversion_compatible(major: int, minor: int):
    """Return if we are compatible to a qt version with `major` and `minor`"""
    qt_version = tuple(map(int, QtCore.__version__.split(".")))
    return qt_version >= (major, minor)


ERROR_DETAILS_DELIM = "\nDetails:\n"  # == GuiServerDevice::m_errorDetailsDelim


def get_reason_parts(failure_reason):
    """
    Try to split the 'failure_reason' into a (short) message and details like
    an exception trace by looking for the ERROR_DETAILS_DELIM.
    If found, return a tuple with
      - the text before the delimiter
      - the text after it (sanitized via html.escape)
    If 'failure_reason' does not contain the delimiter, fall back to use
    'singletons.util.get_error_message(failure_reason)', i.e.
    if that returns its input:
      return (failure_reason, None),
    otherwise
      return (get_error_message(failure_reason), failure_reason)
    """
    both = failure_reason.split(ERROR_DETAILS_DELIM, 1)
    if len(both) == 2:
        return html.escape(both[0]), both[1]
    else:
        msg = get_error_message(failure_reason)
        if msg == failure_reason:
            return msg, None
        else:
            return msg, failure_reason


def convert_npy_to_csv(file_name, parent=None):
    """
    Convert the given file to csv.
    param : file_name: File name as string. It has to be *.npy or *.npz
    """
    file_path = Path(file_name)
    data = np.load(file_name)
    csv_file_name = file_path.with_suffix(".csv")

    if csv_file_name.exists():
        text = (f"The file {csv_file_name} already exists. Do you want to "
                f"overwrite the content?")
        reply = QMessageBox(QMessageBox.Question, "File exists", text,
                            QMessageBox.Yes | QMessageBox.Cancel,
                            parent=parent).exec()
        if reply != QMessageBox.Accepted:
            return
        csv_file_name.unlink()

    if isinstance(data, np.ndarray):
        np.savetxt(csv_file_name, data, fmt="%f", delimiter=",")
    elif isinstance(data, np.lib.npyio.NpzFile):
        with open(csv_file_name, "a") as csv_file:
            for key, array in data.items():
                csv_file.write(f"#{key}\n")
                np.savetxt(csv_file, array, delimiter=",", fmt="%f")
    else:
        message = f"Incorrect numpy file: {file_name}"
        messagebox.show_error(text=message, parent=parent)
        return

    return csv_file_name
