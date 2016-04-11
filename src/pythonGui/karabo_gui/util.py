from uuid import uuid4

from PyQt4.QtGui import QDialog, QFileDialog

from karabo.api_2 import Hash, Schema


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


def getSaveFileName(title, dir="", description="", suffix="", filter=None, selectFile=""):
    dialog = QFileDialog(None, title, dir, description)
    dialog.selectFile(selectFile)
    dialog.setDefaultSuffix(suffix)
    dialog.setFileMode(QFileDialog.AnyFile)
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    if filter is not None:
        dialog.setNameFilter(filter)

    if dialog.exec_() == QDialog.Rejected:
        return
    if len(dialog.selectedFiles()) == 1:
        return dialog.selectedFiles()[0]


def getSchemaModifications(schema, config):
    """ Compute the difference between schema and config.

    Return a Schema containing the differences, or None if there are none.
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

    def dictdiff(d0, d1):
        return {k: v for k, v in d0.items() if v and d1.get(k) != v}

    def addattrs(hsh, path, attrs, schema_attrs):
        hsh[path] = None
        hsh[path, ...] = attrs
        hsh[path, "nodeType"] = schema_attrs["nodeType"]

    modified_schema_hash = Hash()
    for path, attrs in walk(config):
        attrs = nonemptyattrdict(attrs)
        schema_attrs = schema.hash[path, ...]
        if path in schema.hash:
            diff = dictdiff(attrs, schema_attrs)
            if diff:
                addattrs(modified_schema_hash, path, diff, schema_attrs)
        elif attrs:
            addattrs(modified_schema_hash, path, attrs, schema_attrs)

    if not modified_schema_hash.empty():
        return Schema(name=schema.name, hash=modified_schema_hash)

    return None
