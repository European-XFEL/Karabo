#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal, pyqtSlot, QEvent, Qt
from PyQt4.QtGui import QSpinBox

from karabo.middlelayer import Integer
from karabo_gui.displaywidgets.unitlabel import add_unit_label
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class _FocusynSpinBox(QSpinBox):
    """QSpinBox apparently can't be monitored for focus changes with an
    eventFilter. So, we do it the hard way.

    Chemist: 'You can't just go "off" Focusyn!?'
    """
    focusChanged = pyqtSignal(bool)

    def focusInEvent(self, event):
        self.focusChanged.emit(True)
        super(_FocusynSpinBox, self).focusInEvent(event)

    def focusOutEvent(self, event):
        self.focusChanged.emit(False)
        super(_FocusynSpinBox, self).focusOutEvent(event)


class EditableSpinBox(EditableWidget, DisplayWidget):
    category = Integer
    alias = "Integer Spin Box"

    def __init__(self, box, parent):
        super(EditableSpinBox, self).__init__(box)
        self._internal_widget = _FocusynSpinBox(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.focusChanged.connect(self._focusChanged)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

    def setReadOnly(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.installEventFilter(self)
            self._internal_widget.valueChanged.connect(self.onEditingFinished)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    def eventFilter(self, obj, event):
        # Block wheel event on QSpinBox
        return event.type() == QEvent.Wheel and obj is self._internal_widget

    @property
    def value(self):
        return self._internal_widget.value()

    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        self._internal_widget.setRange(max(-0x80000000, rmin),
                                       min(0x7fffffff, rmax))

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)

        if value is None:
            value = 0

        with SignalBlocker(self._internal_widget):
            self._internal_widget.setValue(value)

    @pyqtSlot(bool)
    def _focusChanged(self, has_focus):
        """XXX: Hack around Qt so that the EditableWidget.focusHandler can get
        called. If you can find a way to get focus events for a QSpinBox, this
        messiness can be removed.
        """
        if hasattr(self, 'focusHandler'):
            self.focusHandler(has_focus)
