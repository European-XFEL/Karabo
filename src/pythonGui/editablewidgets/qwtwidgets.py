from widget import EditableWidget

from PyQt4.Qwt5.Qwt import QwtSlider, QwtKnob


class QwtWidget(EditableWidget):
    category = "Digit"

    def __init__(self, box, parent):
        super(QwtWidget, self).__init__(box)
        self.widget = self.Cls(parent)

        self.valueChanged(self.boxes[0], self.boxes[0].value)
        self.widget.valueChanged.connect(self.onEditingFinished)


    def typeChanged(self, box):
        d = box.descriptor
        self.widget.setRange(
            max(getattr(d, 'minInc', None), getattr(d, 'minExc', None), 0),
            max(getattr(d, 'maxInc', None), getattr(d, 'maxExc', None), 100))


    @property
    def value(self):
        return self.widget.value()


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        block = self.widget.blockSignals(True)
        try:
            self.widget.setValue(value)
        finally:
            self.widget.blockSignals(block)


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)


class Slider(QwtWidget):
    alias = "Slider"

    Cls = QwtSlider


class Knob(QwtWidget):
    alias = "Knob"

    Cls = QwtKnob
