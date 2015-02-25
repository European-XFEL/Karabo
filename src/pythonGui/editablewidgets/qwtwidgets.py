#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from util import SignalBlocker
from widget import EditableWidget

from karabo.hashtypes import Number, Integer

from PyQt4.Qwt5.Qwt import QwtSlider, QwtKnob


class QwtWidget(EditableWidget):
    category = Number, Integer

    def __init__(self, box, parent):
        super(QwtWidget, self).__init__(box)
        self.widget = self.Cls(parent)

        self.widget.valueChanged.connect(self.onEditingFinished)


    def typeChanged(self, box):
        d = box.descriptor
        min, max = box.descriptor.getMinMax()
        if box.descriptor.minInc is None and box.descriptor.minExc is None:
            min = 0
        if box.descriptor.maxInc is None and box.descriptor.maxExc is None:
            max = 100
        self.widget.setRange(min, max)


    @property
    def value(self):
        return self.widget.value()


    def valueChanged(self, box, value, timestamp=None):
        with SignalBlocker(self.widget):
            self.widget.setValue(value)


    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, value)


class Slider(QwtWidget):
    alias = "Slider"

    Cls = QwtSlider


class Knob(QwtWidget):
    alias = "Knob"

    Cls = QwtKnob
