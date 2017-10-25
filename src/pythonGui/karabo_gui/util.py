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
from PyQt4.QtCore import QPoint, QSize, Qt, QSettings
from PyQt4.QtGui import (
    QBrush, QDialog, QFileDialog, QHeaderView, QLabel, QMovie, QPainter, QPen,
    QWidget)

from karabo.common.scenemodel.api import read_scene
from karabo.middlelayer import decodeXML, Hash, MetricPrefix, Unit, writeXML
from karabo_gui import globals as krb_globals
from karabo_gui import icons
from karabo_gui import messagebox
from karabo_gui.enums import KaraboSettings
from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.singletons.api import get_db_conn


class PlaceholderWidget(QWidget):
    """A widget which indicates to the user that something is missing or
    unsupported.
    """

    def __init__(self, text, parent=None):
        super(PlaceholderWidget, self).__init__(parent)
        self._text = text

    def paintEvent(self, event):
        with QPainter(self) as painter:
            rect = self.rect()
            boundary = rect.adjusted(2, 2, -2, -2)

            painter.fillRect(rect, QBrush(Qt.lightGray, Qt.FDiagPattern))

            pen = QPen(Qt.lightGray)
            pen.setJoinStyle(Qt.MiterJoin)
            pen.setWidth(4)
            painter.setPen(pen)
            painter.drawRect(boundary)

            metrics = painter.fontMetrics()
            text_rect = metrics.boundingRect(self._text)
            pos = rect.center() - QPoint(text_rect.width() / 2,
                                         -text_rect.height() / 2)
            painter.setPen(QPen())
            painter.drawText(pos, self._text)


class SignalBlocker(object):
    """ Block signals from a QWidget in a with statement """

    def __init__(self, object):
        self.object = object

    def __enter__(self):
        self.state = self.object.blockSignals(True)

    def __exit__(self, a, b, c):
        self.object.blockSignals(self.state)


class WeakMethodRef(object):
    """ A weakref.ref() for bound methods
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


def getSchemaAttributeUpdates(schema, config):
    """ Find all the attributes in a configuration which are different from the
    schema that the configuration is based on.

    Return list of Hashes containing the differences, or None if there are
    none.

    NOTE: `remap_name` and `remap_value` are handling conversion of str
    'Symbol' values to C++ enumeration values (integers). This is because the
    C++ Schema code only knows how to assign from enumeration values, not from
    their stringified representations.
    """
    def walk(h, path=None):
        path = path or []
        for k, v, a in h.iterall():
            yield ".".join(path + [k]), a
            if isinstance(v, Hash):
                for pth, attrs in walk(v, path=path+[k]):
                    yield pth, attrs

    def nonemptyattrdict(d):
        # See the Schema code... These are _always_ added
        special_attrs = ('metricPrefixSymbol', 'unitSymbol')
        return {k: v for k, v in d.items() if k not in special_attrs or v}

    def remap_name(name):
        name_map = {'metricPrefixSymbol': 'metricPrefixEnum',
                    'unitSymbol': 'unitEnum'}
        return name_map.get(name, name)

    def remap_value(name, value):
        symbol_map = {'metricPrefixSymbol': MetricPrefix,
                      'unitSymbol': Unit}
        enum = symbol_map.get(name, None)
        return list(enum).index(enum(value)) if enum else value

    def dictdiff(d0, d1):
        return {k: v for k, v in d0.items() if d1.get(k) != v}

    def get_updates(path, attrs):
        # Format is specified by Device::slotUpdateSchemaAttributes
        return [Hash("path", path, "attribute", k, "value", v)
                for k, v in attrs.items()]

    attr_updates = []
    for path, attrs in walk(config):
        attrs = nonemptyattrdict(attrs)
        if path in schema.hash:
            diff = dictdiff(attrs, schema.hash[path, ...])
            diff = {remap_name(k): remap_value(k, v) for k, v in diff.items()}
            attr_updates.extend(get_updates(path, diff))

    if attr_updates:
        return attr_updates

    return None


def loadConfigurationFromFile(configuration):
    """Given a ``Configuration`` object instance. Read a configuration Hash
    from an XML file and assign it to the object.
    """

    path = get_setting(KaraboSettings.CONFIG_DIR)
    directory = path if path and op.isdir(path) else ""

    filename = getOpenFileName(caption="Open configuration",
                               filter="XML (*.xml)",
                               directory=directory)
    if not filename:
        return

    if configuration is None or configuration.classId is None:
        messagebox.show_error("Configuration load failed")
        return

    with open(filename, 'rb') as fp:
        config = decodeXML(fp.read())

    classId = configuration.classId
    if classId not in config:
        messagebox.show_error("Configuration load failed")
        return
    configuration.dispatchUserChanges(config[classId])
    configuration.signalBoxChanged.emit()

    # Save the directory information
    set_setting(KaraboSettings.CONFIG_DIR, op.dirname(filename))


def saveConfigurationToFile(configuration):
    """This function saves the current configuration of a device to a file.
    """
    if (configuration is None or configuration.classId is None
            or configuration.descriptor is None):
        messagebox.show_error("No configuration available. Saving failed.")
        return

    path = get_setting(KaraboSettings.CONFIG_DIR)
    directory = path if path and op.isdir(path) else ""

    # default configuration name is deviceId
    selectFile = configuration.id.replace('/', '-') + '.xml'

    filename = getSaveFileName(caption="Save configuration as",
                               filter="Configuration (*.xml)",
                               suffix="xml",
                               directory=directory,
                               selectFile=selectFile)
    if not filename:
        return

    hsh, attrs = configuration.toHash()
    classId = configuration.classId
    config = Hash(classId, hsh)
    config[classId, ...] = attrs

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
