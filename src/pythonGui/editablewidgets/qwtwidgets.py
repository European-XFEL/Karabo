from widget import EditableWidget

from PyQt4.Qwt5.Qwt import QwtSlider

class Slider(EditableWidget):
    category = "Digit"
    alias = "Slider"

    def __init__(self, parent=None, value=None, **params):
        super(Slider, self).__init__(**params)

        self.widget = QwtSlider(parent)
        self.valueChanged(self.keys[0], value)
        self.widget.valueChanged.connect(self.onEditingFinished)


    @property
    def value(self):
        return self.widget.value()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0.0

        block = self.widget.blockSignals(True)
        try:
            self.widget.setValue(value)
        finally:
            self.widget.blockSignals(block)


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.keys[0], float(value))
