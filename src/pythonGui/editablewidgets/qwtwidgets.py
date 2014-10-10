#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from util import SignalBlocker
from widget import EditableWidget

from PyQt4.Qwt5.Qwt import QwtSlider, QwtKnob


class QwtWidget(EditableWidget):
    category = "Digit"

    def __init__(self, box, parent):
        super(QwtWidget, self).__init__(box)
        self.widget = self.Cls(parent)

        self.widget.valueChanged.connect(self.onEditingFinished)


    def typeChanged(self, box):
        d = box.descriptor
        self.widget.setRange(
            max(getattr(d, 'minInc', None), getattr(d, 'minExc', None), 0),
            max(getattr(d, 'maxInc', None), getattr(d, 'maxExc', None), 100))


    @property
    def value(self):
        return self.widget.value()


    def valueChanged(self, box, value, timestamp=None):
        with SignalBlocker(self.wiget):
            self.widget.setValue(value)


    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, value)


class Slider(QwtWidget):
    alias = "Slider"

    Cls = QwtSlider


class Knob(QwtWidget):
    alias = "Knob"

    Cls = QwtKnob
