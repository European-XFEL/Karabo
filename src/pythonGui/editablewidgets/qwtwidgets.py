from widget import EditableWidget

from PyQt4.Qwt5.Qwt import QwtSlider

class Slider(EditableWidget):
    category = "Digit"
    alias = "Slider"

    def __init__(self, box, parent):
        super(Slider, self).__init__(box)

        self.widget = QwtSlider(parent)
        d = box.descriptor
        self.widget.setRange(
            max(getattr(d, 'minInc', None), getattr(d, 'minExc', None), 0),
            max(getattr(d, 'maxInc', None), getattr(d, 'maxExc', None), 100))
        self.valueChanged(self.boxes[0], self.boxes[0].value)
        self.widget.valueChanged.connect(self.onEditingFinished)


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
