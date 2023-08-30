import pytest
from qtpy.QtCore import QPoint, Qt
from qtpy.QtTest import QTest
from qtpy.QtWidgets import (
    QDialog, QGraphicsLineItem, QGraphicsPixmapItem, QGraphicsRectItem,
    QGraphicsTextItem)

from karabo.native import Hash, HashList
from karabogui.binding.api import DeviceClassProxy, build_binding
from karabogui.dialogs.logbook_drawing_tools import (
    LineTool, RectTool, TextTool)
from karabogui.dialogs.logbook_preview import LogBookPreview
from karabogui.panels.api import ConfigurationPanel
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.network import Network
from karabogui.testing import get_simple_schema, singletons

title = ("{}: <ConfigurationPanel proxy=<DeviceClassProxy classId=Simple "
         "serverId=test_server>>")


@pytest.fixture
def dialog(gui_app):
    mediator = Mediator()
    with singletons(mediator=mediator):
        panel = ConfigurationPanel()

        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceClassProxy(server_id="test_server", binding=binding)
        panel._show_configuration(proxy)
        dialog = LogBookPreview(parent=panel)
    return dialog


def test_logbook_preview(dialog):
    """Test the logbook preview with a configuration panel"""
    assert dialog.stackedWidget.currentIndex() == 0
    assert dialog.table.columnCount() == 2
    assert dialog.table.rowCount() == 3
    dialog.done(0)


def test_image_preview(dialog, mocker):
    assert dialog.combo_datatype.currentText() == "Image"
    network = Network()
    with singletons(network=network):
        saveLogBook = mocker.patch.object(network, "onSaveLogBook")
        dialog.done(1)
        assert saveLogBook.call_count == 1
        _, args = saveLogBook.call_args
        assert args["dataType"] == "image"
        assert args["title"] == title.format("Image")
        assert args["data"].startswith("data:image/png;base64,")
        assert not args["caption"]


def test_logbook_table_preview(dialog, mocker):
    dialog.combo_datatype.setCurrentIndex(1)
    assert dialog.combo_datatype.currentText() == "Data"
    assert dialog.stackedWidget.currentIndex() == 1
    assert all([checkbox.isChecked() for checkbox in dialog.checkboxes])
    dialog._deselect_all()
    assert not any([checkbox.isChecked() for checkbox in dialog.checkboxes])
    dialog._select_all()
    assert all([checkbox.isChecked() for checkbox in dialog.checkboxes])

    network = Network()
    with singletons(network=network):
        saveLogBook = mocker.patch.object(network, "onSaveLogBook")
        dialog.done(1)
        assert saveLogBook.call_count == 1
        _, args = saveLogBook.call_args
        assert args["dataType"] == "hash"
        assert args["title"] == title.format("Data")
        for item in ("foo", "bar", "charlie"):
            assert item in args["data"]

        # Unselected row should not be saved
        dialog.checkboxes[0].setChecked(False)
        dialog.done(1)
        _, args = saveLogBook.call_args
        data = args["data"]
        assert "foo" not in data
        for item in ("bar", "charlie"):
            assert item in data


def test_save_button(dialog):
    """Test the enabled state of the Save button"""

    assert dialog.ok_button.text() == "Save"
    assert dialog.combo_datatype.currentText() == "Image"
    # Disabled when no proposals.
    assert not dialog.ok_button.isEnabled()

    h_list = HashList()
    h_list.append(Hash())
    dialog._event_destinations(h_list)
    assert dialog.ok_button.isEnabled()

    dialog.combo_datatype.setCurrentIndex(1)
    assert dialog.combo_datatype.currentText() == "Data"
    assert dialog.ok_button.isEnabled()

    # Disabled on deselecting all properties.
    dialog._deselect_all()
    assert not dialog.ok_button.isEnabled()

    # Enabled always for Image.
    dialog.combo_datatype.setCurrentIndex(0)
    assert dialog.combo_datatype.currentText() == "Image"
    assert dialog.ok_button.isEnabled()

    # Enabled when at least one property is selected in the table.
    dialog.combo_datatype.setCurrentIndex(1)
    assert not dialog.ok_button.isEnabled()
    dialog.checkboxes[0].setChecked(True)
    assert dialog.ok_button.isEnabled()

    # Disabled when no property is selected.
    dialog.checkboxes[0].setChecked(False)
    assert not dialog.ok_button.isEnabled()


def test_toolbar(dialog):
    toolbar = dialog.drawing_toolbar
    assert not toolbar.isVisibleTo(dialog)

    dialog.draw_button.setChecked(True)
    assert toolbar.isVisibleTo(dialog)
    actions = toolbar.actions()
    assert len(actions) == 4
    expected = ["Line", "Rect", "Text", "Eraser"]
    assert [action.text() for action in actions] == expected


def test_drawing_tools(dialog, mocker):
    canvas = dialog.canvas
    assert canvas.drawing_tool is None
    assert len(canvas.items()) == 1
    assert isinstance(canvas.items()[0], QGraphicsPixmapItem)

    pos = QPoint(100, 100)
    widget = dialog.view.viewport()
    dialog.draw_button.setChecked(True)
    line_tool = LineTool()
    canvas.set_drawing_tool(line_tool)
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier)
    QTest.mouseMove(widget, pos)
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier)
    assert len(canvas.items()) == 2
    assert isinstance(canvas.items()[0], QGraphicsLineItem)
    assert isinstance(canvas.items()[1], QGraphicsPixmapItem)

    rect_tool = RectTool()
    canvas.set_drawing_tool(rect_tool)
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    QTest.mouseMove(widget, QPoint(200, 200))
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier)
    assert len(canvas.items()) == 3
    assert isinstance(canvas.items()[0], QGraphicsRectItem)
    assert isinstance(canvas.items()[1], QGraphicsLineItem)
    assert isinstance(canvas.items()[2], QGraphicsPixmapItem)

    text_tool = TextTool()
    text_dialog = mocker.patch(
        "karabogui.dialogs.logbook_drawing_tools.TextDialog")
    text_dialog().exec.return_value = QDialog.Accepted
    text_dialog().text = "Hello Karabo"
    canvas.set_drawing_tool(text_tool)
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    assert len(canvas.items()) == 3
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier, pos)
    assert len(canvas.items()) == 4
    assert isinstance(canvas.items()[0], QGraphicsTextItem)
    assert isinstance(canvas.items()[1], QGraphicsRectItem)
    assert isinstance(canvas.items()[2], QGraphicsLineItem)
    assert isinstance(canvas.items()[3], QGraphicsPixmapItem)
    assert canvas.items()[0].toPlainText() == "Hello Karabo"

    # Clear the active drawing tool.
    canvas.set_drawing_tool(None)
    assert canvas.drawing_tool is None
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    QTest.mouseMove(widget, QPoint(200, 200))
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier)
    assert len(canvas.items()) == 4
