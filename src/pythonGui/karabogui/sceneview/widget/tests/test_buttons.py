import pytest
from qtpy.QtCore import QEvent, QPoint, QSize, Qt
from qtpy.QtGui import QMouseEvent

from karabo.common.scenemodel.api import PopupButtonModel
from karabogui.sceneview.widget.api import PopupButtonWidget
from karabogui.widgets.api import TextPopupWidget

TEXT = """
Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod
tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At
vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd
gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet."""


@pytest.fixture()
def poup_button_widget(gui_app):
    model = PopupButtonModel(
        text=TEXT,  x=0, y=0, width=100, height=100)
    widget = PopupButtonWidget(model=model)
    return widget


def test_button_widget(poup_button_widget):
    """Test the popup sticker widget"""

    model = poup_button_widget.model
    widget = poup_button_widget

    size = QSize(model.width, model.height)
    # Check that the size is set correctly and is fixed.
    assert widget.size() == size


def test_popup_widget(poup_button_widget):
    """Verify that the popup widget appears with correc text on mouse click."""
    event = QMouseEvent(QEvent.MouseButtonPress, QPoint(0, 0), Qt.LeftButton,
                        Qt.LeftButton, Qt.NoModifier)
    poup_button_widget.mousePressEvent(event)

    poup_button_widget = poup_button_widget.children()[-1]
    assert poup_button_widget.isVisibleTo(poup_button_widget)
    assert isinstance(poup_button_widget, TextPopupWidget)
    assert poup_button_widget.text_edit.toPlainText() == TEXT
