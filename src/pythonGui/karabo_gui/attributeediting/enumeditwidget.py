from enum import Enum

from PyQt4.QtCore import QEvent, pyqtSlot
from PyQt4.QtGui import QComboBox

from karabo.middlelayer import MetricPrefix, Unit
from karabo_gui.util import SignalBlocker
from .widget import AttributeWidget


def _get_item_text(enum_val):
    # Normalize the ENUM_VALUE_NAME -> Enum value name
    name = enum_val.name.replace('_', ' ').lower().capitalize()
    return "{} ({})".format(enum_val.value, name)


class _DummyEnum(Enum):
    Nil = None


class EnumAttributeEditor(AttributeWidget):
    """ An editor for values defined by an Enum class.
    """
    # Subclasses should define this
    enumClass =_DummyEnum

    def __init__(self, box, parent=None):
        super(EnumAttributeEditor, self).__init__(box, parent)

        instMsg = "Don't instantiate EnumAttributeEditor directly!"
        assert self.enumClass is not _DummyEnum, instMsg

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self._populateWidget()

        self.widget.installEventFilter(self)
        self.widget.currentIndexChanged[int].connect(self.onIndexChanged)

    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        return event.type() == QEvent.Wheel and object is self.widget

    @pyqtSlot(int)
    def onIndexChanged(self, index):
        value = self.widget.itemData(index)
        super(EnumAttributeEditor, self).onEditingFinished(value)

    def attributeValueChanged(self, value):
        if value is None:
            return

        try:
            enum_value = self.enumClass(value)
        except ValueError:
            return

        index = self.widget.findData(enum_value.value)
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
