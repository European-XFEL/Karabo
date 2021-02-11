from PyQt5 import uic
from PyQt5.QtCore import QAbstractItemModel, QModelIndex, Qt
from PyQt5.QtWidgets import QAbstractItemView, QDialog, QHeaderView
from traits.api import (
    Any, Bool, HasStrictTraits, Instance, List, Property, String)

from karabogui.singletons.api import get_config

from .utils import get_dialog_ui

HEADER_LABELS = ['Name', 'Setting']
NAME_COLUMN = 0
SETTING_COLUMN = 1


class ConfigNode(HasStrictTraits):
    # The name of our configuration item
    name = String
    # Our configuration item node
    internal = Any
    # The internally stored value. Can be derived from QSettings
    value = Property
    # The parent node
    parent_node = Instance('ConfigNode', allow_none=True)
    # The list of children
    children = List(Instance('ConfigNode'))
    # Check whether this config node is configurable or not
    editable = Bool

    def _get_value(self):
        if self.internal is None:
            return None
        return "{}".format(get_config()[self.name])

    def _set_value(self, value):
        get_config()[self.name] = value

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

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        node = index.internalPointer()
        if node is None:
            return Qt.NoItemFlags

        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if node.editable:
            ret |= Qt.ItemIsEditable
        return ret

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
            for item in groups[group_name]:
                attr_item = ConfigNode(name=item.name, editable=item.editable,
                                       internal=item, parent_node=group_node)
                group_node.appendChild(attr_item)

    def setData(self, index, value, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role != Qt.EditRole or value is None:
            return False

        # Get the index's stored object
        node = index.internalPointer()
        if node is None:
            return False

        node.value = value

        # A value was successfully set!
        return True


class ConfigurationDialog(QDialog):
    """The dialog to view the configuration singleton
    """

    def __init__(self, parent=None):
        super(ConfigurationDialog, self).__init__(parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        # Don't block the event loop!
        self.setModal(False)
        filepath = get_dialog_ui('configuration.ui')
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
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
