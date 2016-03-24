from enum import Enum

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QComboBox

from karabo.api_2 import MetricPrefix, Unit
from karabo_gui.util import SignalBlocker
from .widget import AttributeWidget


def _get_item_text(enum_val):
    return "{} ({})".format(enum_val.value, enum_val.name)


class _DummyEnum(Enum):
    Nil = None


class EnumAttributeEditor(AttributeWidget):
    """ An editor for values defined by an Enum class.
    """
    # Subclasses should define this
    enumClass =_DummyEnum

    def __init__(self, box, attributeName, parent=None):
        super(EnumAttributeEditor, self).__init__(box, attributeName)

        instMsg = "Don't instantiate EnumAttributeEditor directly!"
        assert self.enumClass is not _DummyEnum, instMsg

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self._populateWidget()

        self.widget.installEventFilter(self)
        self.widget.currentIndexChanged[str].connect(self.onEditingFinished)

    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        return event.type() == QEvent.Wheel and object is self.widget

    @property
    def value(self):
        data = self.widget.itemData(self.widget.currentIndex())
        return data.value()

    def attributeValueChanged(self, value):
        if value is None:
            return

        try:
            enum_value = self.enumClass(value)
        except ValueError:
            return

        text = _get_item_text(enum_value)
        index = self.widget.findText(text)
        if index < 0:
            return

        with SignalBlocker(self.widget):
            self.widget.setCurrentIndex(index)

    def _populateWidget(self):
        for e in self.enumClass:
            text = _get_item_text(e)
            self.widget.addItem(text, e.value)


class UnitAttributeEditor(EnumAttributeEditor):
    """ An EnumAttributeEditor for Unit values.
    """
    enumClass = Unit


class MetricPrefixAttributeEditor(EnumAttributeEditor):
    """ An EnumAttributeEditor for MetricPrefix values.
    """
    enumClass = MetricPrefix
