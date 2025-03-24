import pytest
from qtpy.QtCore import QPoint, Qt
from qtpy.QtGui import QColor, QFont
from qtpy.QtTest import QTest
from qtpy.QtWidgets import (
    QDialog, QGraphicsLineItem, QGraphicsPixmapItem, QGraphicsRectItem,
    QGraphicsSimpleTextItem)

from karabo.native import Hash, HashList
from karabogui.binding.api import DeviceClassProxy, build_binding
from karabogui.dialogs.logbook_drawing_tools import (
    CropTool, LineTool, RectTool, TextTool)
from karabogui.dialogs.logbook_preview import LogBookPreview
from karabogui.panels.api import ConfigurationPanel
from karabogui.singletons.api import get_config
from karabogui.singletons.configuration import Configuration
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.network import Network
from karabogui.testing import get_simple_schema, singletons

TITLE = ("{}: <ConfigurationPanel proxy=<DeviceClassProxy classId=Simple "
         "serverId=test_server>>")


@pytest.fixture(scope="function")
def dialog(gui_app):
    mediator = Mediator()
    config = Configuration()
    with singletons(mediator=mediator, configuration=config):
        panel = ConfigurationPanel()

        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceClassProxy(server_id="test_server", binding=binding)
        panel._show_configuration(proxy)
        dialog = LogBookPreview(parent=panel)
        yield dialog
        dialog._event_network({"status": False})


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
        dialog.combo_title_style.setCurrentIndex(1)
        dialog.done(1)
        assert saveLogBook.call_count == 1
        _, args = saveLogBook.call_args
        assert args["dataType"] == "text_image"
        assert args["title"] == TITLE.format("Image")
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

    dialog.combo_title_style.setCurrentIndex(1)

    network = Network()
    with singletons(network=network):
        saveLogBook = mocker.patch.object(network, "onSaveLogBook")
        dialog.done(1)
        assert saveLogBook.call_count == 1
        _, args = saveLogBook.call_args
        assert args["dataType"] == "hash"
        assert args["title"] == TITLE.format("Data")
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
    h_list.append(Hash("name", "test_stream",
                       "topics", ["Logbook", "Summary"]))
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

    # Disabled when no topic is set
    destination_widget = dialog._destinations[0]
    destination_widget.combo_topic.setCurrentText("")
    assert not dialog.ok_button.isEnabled()


def test_title(dialog, mocker):
    default_title = ("<ConfigurationPanel proxy=<DeviceClassProxy "
                     "classId=Simple serverId=test_server>>")
    assert default_title == dialog.title_line_edit.placeholderText()
    network = Network()
    with singletons(network=network):
        saveLogBook = mocker.patch.object(network, "onSaveLogBook")
        dialog.combo_title_style.setCurrentIndex(1)
        dialog.done(1)
        _, args = saveLogBook.call_args
        assert args["title"] == TITLE.format("Image")
        assert args["headerType"] == "bold"

        custom_title = "Configuration Panel"
        dialog.title_line_edit.setText(custom_title)
        dialog.done(1)
        _, args = saveLogBook.call_args
        assert args["title"] == f"Image: {custom_title}"

        # Table
        dialog.combo_datatype.setCurrentIndex(1)
        dialog.combo_title_style.setCurrentIndex(2)
        dialog.done(1)
        _, args = saveLogBook.call_args
        assert args["title"] == f"Data: {custom_title}"
        assert args["headerType"] == "expandable"

        dialog.title_line_edit.clear()
        dialog.done(1)
        _, args = saveLogBook.call_args
        assert args["title"] == TITLE.format("Data")
        dialog.combo_title_style.setCurrentIndex(0)
        dialog.done(1)
        _, args = saveLogBook.call_args
        assert args["title"] == TITLE.format("Data")


def test_toolbar(dialog):
    toolbar = dialog.drawing_toolbar
    assert toolbar.isVisibleTo(dialog)
    actions = toolbar.actions()
    assert len(actions) == 7
    # First item is a label with  warning text
    expected = ["", "Change annotation color", "Draw Line", "Draw Rectangle",
                "Add Text to the Image", "Delete item", "Crop image"]
    assert [action.toolTip() for action in actions] == expected


def test_drawing_tools(dialog, mocker):
    canvas = dialog.canvas
    assert canvas.drawing_tool is None
    assert len(canvas.items()) == 1
    assert isinstance(canvas.items()[0], QGraphicsPixmapItem)

    can_draw = mocker.patch.object(dialog.canvas, "_can_draw")
    can_draw.return_value = True

    pos = QPoint(100, 100)
    widget = dialog.view.viewport()
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
    text_dialog().text_font = QFont()
    text_dialog().text_color = QColor("black")
    text_dialog().text = "Hello Karabo"
    canvas.set_drawing_tool(text_tool)
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    assert len(canvas.items()) == 3
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier, pos)
    assert len(canvas.items()) == 4
    assert isinstance(canvas.items()[0], QGraphicsSimpleTextItem)
    assert isinstance(canvas.items()[1], QGraphicsRectItem)
    assert isinstance(canvas.items()[2], QGraphicsLineItem)
    assert isinstance(canvas.items()[3], QGraphicsPixmapItem)
    assert canvas.items()[0].text() == "Hello Karabo"

    # Clear the active drawing tool.
    canvas.set_drawing_tool(None)
    assert canvas.drawing_tool is None
    can_draw.return_value = False
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    QTest.mouseMove(widget, QPoint(200, 200))
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier)
    assert len(canvas.items()) == 4

    tool = CropTool()
    canvas.set_drawing_tool(tool)
    assert canvas.drawing_tool is not None
    can_draw.return_value = False
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, pos)
    assert canvas.drawing_tool is None


def test_topics(dialog):
    """Test the topics combobox is updated when the stream selection changes"""
    h_list = HashList()
    stream1 = "12345"
    topics1 = ["Logbook", "Setup", "Day Shift"]

    stream2 = "34567"
    topics2 = ["Data Analysis", "Detector status"]

    h_list.append(Hash("name", stream1, "topics", topics1))
    h_list.append(Hash("name", stream2, "topics", topics2))
    dialog._event_destinations(h_list)
    destination_widget = dialog._destinations[0]
    destination_widget.combo_stream.setCurrentText(stream2)
    combo = destination_widget.combo_topic
    assert [combo.itemText(i) for i in range(combo.count())] == topics2

    destination_widget.combo_stream.setCurrentText(stream1)
    assert [combo.itemText(i) for i in range(combo.count())] == topics1


def test_restore_stream_topic(dialog):
    h_list = HashList()
    stream1 = "12345"
    topics1 = ["Logbook", "Setup", "Day Shift"]

    stream2 = "34567"
    topics2 = ["Data Analysis", "Summary", "Detector status",
               "Sample status", "Beam Status"]

    stream3 = "98765"
    topics3 = ["Logbook", "RunSummary", "ShiftSummary"]

    h_list.append(Hash("name", stream1, "topics", topics1))
    h_list.append(Hash("name", stream2, "topics", topics2))
    h_list.append(Hash("name", stream3, "topics", topics3))

    default_stream = "34567"
    default_topic = "Summary"
    get_config()["logbook_stream"] = default_stream
    get_config()["logbook_topic"] = default_topic

    dialog._event_destinations(h_list)

    destination_widget = dialog._destinations[0]
    assert destination_widget.combo_stream.currentText() == default_stream
    assert destination_widget.combo_topic.currentText() == default_topic


def test_create_new_topic(dialog):
    """
    Test new topic creation. Verify that create_topic button
    enables/disables the edit mode on topic combobox.
    """
    destination_widget = dialog._destinations[0]
    destination_widget.create_topic.clicked.emit(True)
    assert destination_widget.combo_topic.isEditable()

    line_edit = destination_widget.combo_topic.lineEdit()
    line_edit.setText("NewTopic")
    destination_widget.combo_topic.lineEdit().editingFinished.emit()
    assert destination_widget.combo_topic.findText("NewTopic") != -1
    assert destination_widget.combo_topic.currentText() == "NewTopic"

    destination_widget.create_topic.setChecked(False)
    assert not destination_widget.combo_topic.isEditable()

    # Do not allow empty topic
    destination_widget.create_topic.clicked.emit(True)
    destination_widget.combo_topic.lineEdit().setText("")
    destination_widget.combo_topic.lineEdit().editingFinished.emit()
    assert destination_widget.combo_topic.currentText() == "NewTopic"


def test_concurrency(dialog):
    """
    The KaraboEvent.ActiveDestinations should not duplicate the streams
    in the combobox. This happens when multiple dialogs are opened.
    """
    h_list = HashList()
    h_list.append(Hash("name", "test_stream",
                       "topics", ["Logbook", "Summary"]))
    dialog._event_destinations(h_list)
    destination_widget = dialog._destinations[0]
    assert destination_widget.combo_stream.count() == 1

    dialog._event_destinations(h_list)
    assert destination_widget.combo_stream.count() == 1
