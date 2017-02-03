from contextlib import contextmanager
import os
import os.path as op
from tempfile import mkstemp
from uuid import uuid4

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QDialog, QFileDialog, QLabel, QMovie

from karabo.middlelayer import Hash, MetricPrefix, Unit
import karabo_gui.globals as globals
import karabo_gui.icons as icons
from karabo_gui.messagebox import MessageBox
from karabo_gui.singletons.api import get_db_conn


class SignalBlocker(object):
    """ Block signals from a QWidget in a with statement """
    def __init__(self, object):
        self.object = object

    def __enter__(self):
        self.state = self.object.blockSignals(True)

    def __exit__(self, a, b, c):
        self.object.blockSignals(self.state)


def generateObjectName(widget):
    return "{0}_{1}".format(widget.__class__.__name__, uuid4().hex)


def getOpenFileName(parent=None, caption="", filter=""):
    """ Return `filename` of the Qt file open dialog.
    """
    return QFileDialog.getOpenFileName(parent=parent,
                                       caption=caption,
                                       directory=globals.HIDDEN_KARABO_FOLDER,
                                       filter=filter,
                                       options=QFileDialog.DontUseNativeDialog)


def getSaveFileName(parent=None, caption="", directory="", filter="",
                    suffix="", selectFile=""):
    if not directory:
        directory = globals.HIDDEN_KARABO_FOLDER

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
    filename = getOpenFileName(caption="Open configuration",
                               filter="XML (*.xml)")
    if not filename:
        return

    if configuration is None or configuration.classId is None:
        MessageBox.showError("Configuration load failed")
        return

    with open(filename, 'rb') as fp:
        config = Hash.decode(fp.read(), 'XML')

    classId = configuration.classId
    if classId not in config:
        MessageBox.showError("Configuration load failed")
        return
    configuration.fromHash(config[classId])


def saveConfigurationToFile(configuration):
    """This function saves the current configuration of a device to a file.
    """
    filename = getSaveFileName(caption="Save configuration as",
                               filter="Configuration (*.xml)",
                               suffix="xml")
    if not filename:
        return

    if configuration is None or configuration.classId is None:
        MessageBox.showError("Configuration save failed")
        return

    hsh, attrs = configuration.toHash()
    classId = configuration.classId
    config = Hash(classId, hsh)
    config[classId, ...] = attrs

    # Save configuration to file
    with open(filename, 'wb') as fp:
        fp.write(config.encode('XML'))


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


def is_database_processing():
    """Return whether database is currently processing something. A message box
    is shown if the processing is active.
    """
    # Make sure there are not pending DB things in the pipe
    if get_db_conn().is_processing():
        msg = ('There is currently data fetched from or sent to the <br>'
               '<b>project database</b>. Please wait until this is done!')
        MessageBox.showWarning(msg, 'Database connection active')
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
