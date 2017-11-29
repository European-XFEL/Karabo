from contextlib import contextmanager
from datetime import datetime
import os
import os.path as op
from io import StringIO
from tempfile import mkstemp
from types import MethodType
from uuid import uuid4
import weakref

from dateutil.tz import tzlocal, tzutc
from PyQt4.QtCore import QEvent, QObject, QSize, QSettings
from PyQt4.QtGui import QDialog, QFileDialog, QHeaderView, QLabel, QMovie

from karabo.common.scenemodel.api import read_scene
from karabo.middlelayer import decodeXML, Hash, writeXML
from karabogui import globals as krb_globals, icons, messagebox
from karabogui.binding.api import apply_configuration, extract_configuration
from karabogui.enums import KaraboSettings
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.singletons.api import get_db_conn


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


def loadConfigurationFromFile(device_proxy):
    """Given a ``BaseDeviceProxy`` object instance. Read a configuration Hash
    from an XML file and assign it to the object.
    """
    path = get_setting(KaraboSettings.CONFIG_DIR)
    directory = path if path and op.isdir(path) else ""

    filename = getOpenFileName(caption="Open configuration",
                               filter="XML (*.xml)",
                               directory=directory)
    if not filename:
        return

    if device_proxy is None or device_proxy.binding is None:
        messagebox.show_error("Configuration load failed")
        return

    with open(filename, 'rb') as fp:
        config = decodeXML(fp.read())

    binding = device_proxy.binding
    if binding.class_id not in config:
        messagebox.show_error("Configuration load failed")
        return

    apply_configuration(config[binding.class_id], binding)
    # Save the directory information
    set_setting(KaraboSettings.CONFIG_DIR, op.dirname(filename))


def saveConfigurationToFile(device_proxy):
    """This function saves the current configuration of a device to a file.
    """
    if device_proxy is None or device_proxy.binding is None:
        messagebox.show_error("No configuration available. Saving failed.")
        return

    path = get_setting(KaraboSettings.CONFIG_DIR)
    directory = path if path and op.isdir(path) else ""

    # default configuration name is classId
    selectFile = device_proxy.binding.class_id + '.xml'

    filename = getSaveFileName(caption="Save configuration as",
                               filter="Configuration (*.xml)",
                               suffix="xml",
                               directory=directory,
                               selectFile=selectFile)
    if not filename:
        return

    class_id = device_proxy.binding.class_id
    config = Hash(class_id, extract_configuration(device_proxy.binding))
    # XXX: Copy attributes as well?

    # Save configuration to file
    with open(filename, 'w') as fp:
        writeXML(config, fp)

    # save the last config directory
    set_setting(KaraboSettings.CONFIG_DIR, op.dirname(filename))


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


def handle_scene_from_server(dev_id, name, project, success, reply):
    """Callback handler for a request to a device to load one of its scenes.
    """
    if not (success and reply.get('payload.success', False)):
        msg = 'Scene "{}" from device "{}" was not retreived!'
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
    event_type = KaraboEventSender.ShowUnattachedSceneView
    if project is not None:
        event_type = KaraboEventSender.ShowSceneView
        project.scenes.append(scene)
    broadcast_event(event_type, {'model': scene})


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


def get_spin_widget(scaled_size=QSize(), parent=None):
    """Return a ``QLabel`` containing a spinning icon.
    """
    spin_widget = QLabel(parent)
    movie = QMovie(op.join(op.abspath(op.dirname(icons.__file__)), 'wait'))
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


def get_setting(attr):
    """ This function is used to retrieve a value from the QSettings file """
    assert isinstance(attr, KaraboSettings)
    return QSettings().value(attr.name)


def set_setting(attr, value):
    """ This function is used to set an attribute in the QSettings file """
    assert isinstance(attr, KaraboSettings)
    QSettings().setValue(attr.name, value)


def utc_to_local(utc_str, format='%Y-%m-%d %H:%M:%S'):
    """Convert given `utc_str` in a given `format` to the local time string
    """
    if not utc_str:
        return ''

    utc_ts = datetime.strptime(utc_str, format)
    local_ts = utc_ts.replace(tzinfo=tzutc()).astimezone(tzlocal())
    return datetime.strftime(local_ts, format)
