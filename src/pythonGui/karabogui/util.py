from contextlib import contextmanager
from datetime import datetime
from functools import partial
from io import StringIO
import os
import os.path as op
import re
from tempfile import mkstemp
from types import MethodType
from uuid import uuid4
import weakref

from dateutil.tz import tzlocal, tzutc
from PyQt4.QtCore import QEvent, QObject, Qt, QSize
from PyQt4.QtGui import (
    QApplication, QCursor, QDialog, QFileDialog, QHeaderView, QLabel, QMovie,
    QValidator)

from karabo.common.project.api import read_macro
from karabo.common.scenemodel.api import SceneTargetWindow, read_scene
from karabo.native import decodeXML, Hash, writeXML
from karabogui import globals as krb_globals, icons, messagebox
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, extract_configuration)
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.request import call_device_slot
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

    return QFileDialog.getOpenFileName(parent=parent,
                                       caption=caption,
                                       directory=directory,
                                       filter=filter,
                                       options=QFileDialog.DontUseNativeDialog)


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


def load_configuration_from_file(device_proxy):
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
                               directory=directory)
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


def save_configuration_to_file(device_proxy):
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
                               selectFile=default_name)
    if not filename:
        return

    # If we are shutdown in between, either server or device, notify!
    if device_proxy.binding is None or not len(device_proxy.binding.value):
        messagebox.show_error("The configuration is empty and cannot "
                              "be saved", title='No configuration')
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


def get_scene_from_server(device_id, scene_name, project=None,
                          target_window=SceneTargetWindow.Dialog):
    """Get a scene from the a device

    :param device_id: The deviceId of the device
    :param scene_name: The scene name
    :param project: The project owner of the scene. Default is None.
    :param target_window: The target window option. Default is Dialog.
    """

    handler = partial(handle_scene_from_server, device_id, scene_name,
                      project, target_window)
    call_device_slot(handler, device_id, 'requestScene',
                     name=scene_name)


def handle_scene_from_server(dev_id, name, project, target_window, success,
                             reply):
    """Callback handler for a request to a device to load one of its scenes.
    """
    if not (success and reply.get('payload.success', False)):
        msg = 'Scene "{}" from device "{}" was not retrieved!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Scene from Device Failed')
        return

    data = reply.get('payload.data', '')
    if not data:
        msg = 'Scene "{}" from device "{}" contains no data!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Scene from Device Failed')
        return

    with StringIO(data) as fp:
        scene = read_scene(fp)
        scene.modified = True
        scene.simple_name = '{}|{}'.format(dev_id, name)
        scene.reset_uuid()

    # Add to the project AND open it
    event_type = KaraboEvent.ShowUnattachedSceneView
    window = SceneTargetWindow.MainWindow
    if target_window is not None:
        window = target_window
    if project is not None:
        event_type = KaraboEvent.ShowSceneView
        project.scenes.append(scene)
    broadcast_event(event_type, {'model': scene, 'target_window': window})


def handle_macro_from_server(dev_id, name, project, success, reply):
    if not (success and reply.get('payload.success', False)):
        msg = 'Macro "{}" from device "{}" was not retrieved!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Macro from Device Failed')
        return

    data = reply.get('payload.data', '')
    if not data:
        msg = 'Macro "{}" from device "{}" contains no data!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Macro from Device Failed')
        return

    with StringIO(data) as fp:
        macro = read_macro(fp)
        macro.initialized = macro.modified = True
        macro.simple_name = '{}-{}'.format(dev_id, name)
        macro.reset_uuid()

    # Macro's can only be added to project. Hence, add first to the project
    project.macros.append(macro)
    # and then open it
    broadcast_event(KaraboEvent.ShowMacroView, {'model': macro})


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


def set_treeview_header(tree_view):
    """This function is used by the ``QTreeView`` used for the navigation and
    the projects and sets its header correctly.
    """
    # NOTE: Since QTreeView always displays the expander in column 0 the
    # additional columns are moved to the front
    tree_view.header().moveSection(1, 0)
    tree_view.header().moveSection(2, 0)

    tree_view.header().setResizeMode(0, QHeaderView.ResizeToContents)
    tree_view.header().setResizeMode(1, QHeaderView.Fixed)
    tree_view.header().setResizeMode(2, QHeaderView.Fixed)
    tree_view.setColumnWidth(1, 20)
    tree_view.setColumnWidth(2, 20)

    # Prevent drag reorder of the header
    tree_view.header().setMovable(False)


def utc_to_local(utc_str, format='%Y-%m-%d %H:%M:%S'):
    """Convert given `utc_str` in a given `format` to the local time string
    """
    if not utc_str:
        return ''

    utc_ts = datetime.strptime(utc_str, format)
    local_ts = utc_ts.replace(tzinfo=tzutc()).astimezone(tzlocal())
    return datetime.strftime(local_ts, format)


class InputValidator(QValidator):
    """ This class provides validation of user inputs within projects, whether
        it is about deviceIds, macro names, or scene names.

        The naming convention is as follows:
        part_one[/optional_part_two[/optional_part_three[/and_so_on]]]
        '-' sign is also allowed
    """
    def __init__(self, parent=None):
        QValidator.__init__(self, parent)

    pattern = re.compile(r'^[\w-]+(/[\w-]+)*$')

    def validate(self, input, pos):
        if not input or input.endswith('/'):
            return self.Intermediate, input, pos

        if not self.pattern.match(input):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos
