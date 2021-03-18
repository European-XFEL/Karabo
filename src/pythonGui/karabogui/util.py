from contextlib import contextmanager
from datetime import datetime
import os
import os.path as op
import re
from tempfile import mkstemp
from types import MethodType
from uuid import uuid4
import webbrowser
import weakref

from dateutil.tz import tzlocal, tzutc
from PyQt5.QtCore import QEvent, QEventLoop, QObject, Qt, QSize
from PyQt5.QtWidgets import (
    QApplication, QDialog, QFileDialog, QHeaderView, QLabel)
from PyQt5.QtGui import QCursor, QMovie, QValidator

from karabo.native import decodeXML, Hash, writeXML
from karabogui import globals as krb_globals, icons, messagebox
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, extract_configuration)
from karabogui.const import PANEL_ICON_SIZE
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.singletons.api import get_config, get_db_conn


class MouseWheelEventBlocker(QObject):
    """A QObject which can be used for event filtering of mouse wheel events.
    """

    def __init__(self, widget):
        super(MouseWheelEventBlocker, self).__init__()
        self.widget = widget

    def eventFilter(self, obj, event):
        # Block wheel events
        return event.type() == QEvent.Wheel and obj is self.widget


class SignalBlocker(object):
    """Block signals from a QWidget in a with statement"""

    def __init__(self, object):
        self.object = object

    def __enter__(self):
        self.state = self.object.blockSignals(True)

    def __exit__(self, a, b, c):
        self.object.blockSignals(self.state)


class WeakMethodRef(object):
    """A weakref.ref() for bound methods
    """

    def __init__(self, bound_method, num_args=-1):
        # Preconditions...
        # bound_method MUST be a bound method
        assert type(bound_method) is MethodType
        if num_args > -1:
            # bound_method MUST take N args (- 1 because of self)!
            needed_args = bound_method.__func__.__code__.co_argcount - 1
            assert needed_args == num_args

        obj = bound_method.__self__
        if obj is not None:
            self.obj = weakref.ref(obj, self._owner_deleted)
            self.name = bound_method.__name__

    def __call__(self, *args, **kwargs):
        if self.obj is not None:
            obj = self.obj()
            if obj is not None:
                method = getattr(obj, self.name)
                method(*args, **kwargs)

    def _owner_deleted(self, ref):
        self.obj = None


def generateObjectName(widget):
    return "{0}_{1}".format(widget.__class__.__name__, uuid4().hex)


def getOpenFileName(parent=None, caption="", filter="", directory=""):
    """ Return `filename` of the Qt file open dialog.
    """
    directory = directory or krb_globals.HIDDEN_KARABO_FOLDER

    filename, _ = QFileDialog.getOpenFileName(
        parent=parent,
        caption=caption,
        directory=directory,
        filter=filter,
        options=QFileDialog.DontUseNativeDialog)

    return filename


def getSaveFileName(parent=None, caption="", filter="", directory="",
                    suffix="", selectFile=""):
    directory = directory or krb_globals.HIDDEN_KARABO_FOLDER

    dialog = QFileDialog(parent, caption, directory, filter)
    dialog.selectFile(selectFile)
    dialog.setDefaultSuffix(suffix)
    dialog.setFileMode(QFileDialog.AnyFile)
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    if filter:
        dialog.setNameFilter(filter)

    if dialog.exec_() == QDialog.Rejected:
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
    path = config['config_dir']
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
    get_config()['config_dir'] = op.dirname(filename)
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
    path = config['config_dir']
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
    get_config()['config_dir'] = op.dirname(filename)


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
    pos = QCursor.pos()
    pos.setX(pos.x() + 10)
    pos.setY(pos.y() + 10)
    widget.move(pos)


def process_qt_events(app=None, timeout=100):  # ms
    if app is None:
        app = QApplication

    flags = QEventLoop.AllEvents | QEventLoop.WaitForMoreEvents
    app.processEvents(flags, timeout)
