from qtpy.QtWidgets import QDialog, QToolButton

from karabo.native import Configurable, VectorDouble
from karabogui.testing import click_button, get_property_proxy

from ..widgets import TableVectorEdit


class TableSchema(Configurable):
    vector = VectorDouble(
        displayedName="VectorDouble",
        displayType="TableVector",
        defaultValue=[])


def test_table_vector_edit(gui_app, mocker):
    proxy = get_property_proxy(TableSchema.getClassSchema(), "vector")
    widget = TableVectorEdit(proxy.binding)
    assert widget is not None
    widget.setText("2.1,2")
    assert widget.list_edit.values == ["2.1", "2"]
    assert widget.list_edit.string_values == "2.1,2"

    # Set a new value
    dialog_instance = mocker.Mock()
    dialog_instance.exec.return_value = QDialog.Accepted
    dialog_instance.string_values = "4.1"

    mocker.patch.object(widget, "list_edit", new=dialog_instance)
    button = widget.layout().itemAt(1).widget()
    assert isinstance(button, QToolButton)
    click_button(button)
    dialog_instance.exec.assert_called_once()
    assert widget.text() == "4.1"
