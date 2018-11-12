import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import QAbstractItemModel, QModelIndex, Qt
from PyQt4.QtGui import QAbstractItemView, QDialog, QHeaderView
from traits.api import (
    Any, HasStrictTraits, Instance, List, Property, String)

from karabogui.singletons.api import get_config

HEADER_LABELS = ['Name', 'Setting']
NAME_COLUMN = 0
SETTING_COLUMN = 1


class ConfigNode(HasStrictTraits):
    # The name of our configuration item
    name = String
    # The internally stored value. Can be derived from QSettings
    internal = Any
    # The presented value to the outside
    value = Property
    # The parent node
    parent_node = Instance('ConfigNode', allow_none=True)
    children = List(Instance('ConfigNode'))

    def _get_value(self):
        if self.internal is None:
            return None
        return "{}".format(self.internal)

    def _set_value(self, value):
        self.internal = value

    def appendChild(self, item):
        self.children.append(item)

    def child(self, row):
        return self.children[row]

    def childCount(self):
        return len(self.children)

    def columnCount(self):
        return len(HEADER_LABELS)

    def row(self):
        if self.parent_node is None:
            return 0
        return self.parent_node.children.index(self)

    def parent(self):
        return self.parent_node


class ConfigurationModel(QAbstractItemModel):
    def __init__(self, parent=None):
        super(ConfigurationModel, self).__init__(parent)
        self.root = ConfigNode()
        self._get_model_data()

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractItemModel
        """
        return len(HEADER_LABELS)

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel
        """
        if not index.isValid():
            return None

        item = index.internalPointer()
        column = index.column()
        if column == NAME_COLUMN:
            if role == Qt.DisplayRole:
                return item.name
        elif column == SETTING_COLUMN:
            if role == Qt.DisplayRole:
                return item.value

    def headerData(self, column, orientation, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel
        """
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return HEADER_LABELS[column]

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel
        """
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_node = self.root
        else:
            parent_node = parent.internalPointer()

        child_node = parent_node.child(row)
        if child_node is not None:
            return self.createIndex(row, column, child_node)

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel
        """
        if not index.isValid():
            return QModelIndex()

        child_node = index.internalPointer()
        if not child_node:
            return QModelIndex()

        parent_node = child_node.parent()

        if parent_node == self.root:
            return QModelIndex()

        return self.createIndex(parent_node.row(), 0, parent_node)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel
        """
        if parent.column() > 0:
            return 0
        if not parent.isValid():
            parent = self.root
        else:
            parent = parent.internalPointer()
        return parent.childCount()

    def _get_model_data(self):
        """Setup the model data from the configuration singleton
        """
        config = get_config()
        groups = config.groups()
        for group_name in groups.keys():
            group_node = ConfigNode(name=group_name,
                                    parent_node=self.root)
            self.root.appendChild(group_node)
            for attr in groups[group_name]:
                attr_item = ConfigNode(name=attr, value=getattr(config, attr),
                                       parent_node=group_node)
                group_node.appendChild(attr_item)


class ConfigurationDialog(QDialog):
    """The dialog to view the configuration singleton
    """

    def __init__(self, parent=None):
        super(ConfigurationDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'configuration.ui')
        # Don't block the event loop!
        self.setModal(False)
        uic.loadUi(filepath, self)

        self.tree_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        item_model = ConfigurationModel(parent=self)
        self.tree_view.setModel(item_model)
        self.tree_view.setAlternatingRowColors(True)
        self.configure_header()

    def configure_header(self):
        """Configure the treeview header to automatically resize its contents
        """
        header = self.tree_view.header()
        header.setResizeMode(0, QHeaderView.ResizeToContents)
        header.setResizeMode(1, QHeaderView.ResizeToContents)
