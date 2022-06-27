import html
import os
import os.path as op
import re
import webbrowser
from contextlib import contextmanager
from datetime import datetime
from tempfile import mkstemp
from uuid import uuid4

from dateutil.tz import tzlocal, tzutc
from qtpy import QtCore
from qtpy.QtCore import QEvent, QEventLoop, QObject, QSize, Qt
from qtpy.QtGui import QCursor, QMovie, QValidator
from qtpy.QtWidgets import (
    QApplication, QDialog, QFileDialog, QHeaderView, QLabel)

from karabo.native import Hash, decodeXML, writeXML
from karabogui import const, icons, messagebox
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, extract_configuration)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_config, get_db_conn
from karabogui.singletons.util import get_error_message


class MouseWheelEventBlocker(QObject):
    """A QObject which can be used for event filtering of mouse wheel events.
    """

    def __init__(self, widget):
        super(MouseWheelEventBlocker, self).__init__()
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
    return "{0}_{1}".format(widget.__class__.__name__, uuid4().hex)


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

    config = Hash(class_id, extract_configuration(device_proxy.binding,
                                                  include_attributes=True))

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


def set_treeview_header(header):
    """This function is used by the ``QTreeView`` used for the navigation and
    the projects and sets its header correctly.
    """
    # NOTE: Since QTreeView always displays the expander in column 0 the
    # additional columns are moved to the front
    header.moveSection(1, 0)
    header.moveSection(2, 0)
    header.setSectionResizeMode(0, QHeaderView.Stretch)
    header.setSectionResizeMode(1, QHeaderView.Fixed)
    header.setSectionResizeMode(2, QHeaderView.Fixed)
    header.setStretchLastSection(False)
    PANEL_ICON_SIZE = 26
    header.setMaximumSectionSize(PANEL_ICON_SIZE)
    header.resizeSection(1, PANEL_ICON_SIZE)
    header.resizeSection(2, PANEL_ICON_SIZE)

    # Prevent drag reorder of the header
    header.setSectionsMovable(False)


def utc_to_local(utc_str, format='%Y-%m-%d %H:%M:%S'):
    """Convert given `utc_str` in a given `format` to the local time string
    """
    if not utc_str:
        return ''

    utc_ts = datetime.strptime(utc_str, format)
    local_ts = utc_ts.replace(tzinfo=tzutc()).astimezone(tzlocal())
    return datetime.strftime(local_ts, format)


VALID_PROJECT_OBJECT_NAME = re.compile(r'^[\w-]+(/[\w-]+)*$')


class InputValidator(QValidator):
    """ This class provides validation of user inputs within projects, whether
        it is about deviceIds, macro names, or scene names.

        The naming convention is as follows:
        part_one[/optional_part_two[/optional_part_three[/and_so_on]]]
        '-' sign is also allowed
    """

    def __init__(self, parent=None):
        QValidator.__init__(self, parent)

    def validate(self, input, pos):
        if not input or input.endswith('/'):
            return self.Intermediate, input, pos

        if not VALID_PROJECT_OBJECT_NAME.match(input):
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


def open_documentation_link(deviceId):
    """Return the valid configuration link for a deviceId

    NOTE: The respective karabo topic and deviceId must be provided
    in the raw documentation template

    :param deviceId: The deviceId of the instance
    """
    configuration = get_config()
    web_link = configuration['documentation']

    # XXX: The topic is lower case
    topic = configuration['broker_topic'].lower()
    # XXX: This configuration link can be edited. We protect ourselves here!
    try:
        url = web_link.format(topic=topic, deviceId=deviceId)
    except KeyError:
        messagebox.show_error("The documentation link is wrongly formatted:\n"
                              "{}".format(web_link))
        return

    try:
        webbrowser.open_new(url)
    except webbrowser.Error:
        messagebox.show_error("No web browser available!")


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


def process_qt_events(app=None, timeout=100):  # ms
    if app is None:
        app = QApplication

    flags = QEventLoop.AllEvents | QEventLoop.WaitForMoreEvents
    app.processEvents(flags, timeout)


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
