#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QDial, QSlider

from karabo.middlelayer import Number, Integer
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget


class _AnalogEditorWidget(EditableWidget):
    category = Number, Integer

    def typeChanged(self, box):
        d = box.descriptor
        min, max = d.getMinMax()
        if d.minInc is None and d.minExc is None:
            min = 0
        if d.maxInc is None and d.maxExc is None:
            max = 100
        self.widget.setRange(min, max)

    @property
    def value(self):
        return self.widget.value()

    def valueChanged(self, box, value, timestamp=None):
        with SignalBlocker(self.widget):
            self.widget.setValue(value)


class Slider(_AnalogEditorWidget):
    alias = "Slider"

    def __init__(self, box, parent):
        super(Slider, self).__init__(box)
        slider = QSlider(Qt.Horizontal, parent)
        slider.setTickPosition(QSlider.TicksBelow)
        slider.setFocusPolicy(Qt.StrongFocus)
        self.widget = slider
        self.widget.valueChanged.connect(self.onEditingFinished)


class Knob(_AnalogEditorWidget):
    alias = "Knob"

    def __init__(self, box, parent):
        super(Knob, self).__init__(box)
        dial = QDial(parent)
        dial.setNotchesVisible(True)
        dial.setFocusPolicy(Qt.StrongFocus)
        self.widget = dial
        self.widget.valueChanged.connect(self.onEditingFinished)
