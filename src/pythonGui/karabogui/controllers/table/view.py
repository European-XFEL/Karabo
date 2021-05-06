import json

from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtWidgets import QTableView

from karabogui.binding.api import StringBinding
from karabogui.enums import NavigationItemTypes, ProjectItemTypes


class KaraboTableView(QTableView):
    """The KaraboTableView supports `Drag'N'Drop` operations of devices
    from the Navigation or Project Panel.

    Dropping a device item on a string binding will replace the existing value
    with the `deviceId`.
    Dropping the device item in a free region or not a string binding will
    append a row to the existing table. The first found string binding of
    the table is edited with the `deviceId`.

    Note: For both operations a string binding must be available.
    """

    def __init__(self, parent=None):
        super(KaraboTableView, self).__init__(parent)
        self._header = None
        self._bindings = None
        self._drag_column = None

    def set_bindings(self, bindings):
        self._bindings = bindings
        self._header = list(bindings.keys())
        # Note: Evaluate the eventual `drag-column`. If a string element is
        # found in the row schema, the first appearance is taken!
        for index, key in enumerate(self._header):
            binding = bindings[key]
            if isinstance(binding, StringBinding):
                self._drag_column = index
                break

    def dragEnterEvent(self, event):
        self._check_drag_event(event)

    def dragMoveEvent(self, event):
        self._check_drag_event(event)

    def dropEvent(self, event):
        success, data = self._check_drag_event(event)
        if success:
            index, new_row, deviceId = data
            model = self.model()
            if new_row:
                # Having a `new_row` must have a drag column
                if self._drag_column is not None:
                    model.insertRows(model.rowCount(), 1, QModelIndex())
                    index = model.index(model.rowCount() - 1,
                                        self._drag_column,
                                        QModelIndex())
                    # Scroll to bottom to drop new item!
                    self.scrollToBottom()
                    model.setData(index, deviceId, Qt.EditRole)
            else:
                # Plainly insert the deviceId at the given cell index!
                model.setData(index, deviceId, Qt.EditRole)

    def _check_drag_event(self, event):
        """Check the drag event for enter, drag, and drop events.

        This method returns a success boolean and a tuple of meta data when
        successful (index: QModelIndex, new_row: bool, deviceId: str)
        """
        if self._header is None:
            # Note: Header is only present after the schema has arrived
            return False, ()

        items = event.mimeData().data('treeItems').data()
        if not items:
            event.ignore()
            return False, ()

        # Check if a device was dragged in the item
        item = json.loads(items.decode())[0]
        item_type = item.get('type')
        from_navigation = item_type == NavigationItemTypes.DEVICE
        from_project = item_type == ProjectItemTypes.DEVICE
        from_device = from_navigation or from_project
        deviceId = item.get('deviceId', 'None')

        # No cell drop, must create a new row!
        index = self.indexAt(event.pos())
        if not index.isValid() and from_device:
            event.accept()
            return True, (index, True, deviceId)

        key = self._header[index.column()]
        binding = self._bindings[key]
        if from_device:
            event.accept()
            # If the selected cell is not a string, we have to insert a new
            # row with the data!
            new_row = not isinstance(binding, StringBinding)
            return True, (index, new_row, deviceId)

        event.ignore()
        return False, ()
