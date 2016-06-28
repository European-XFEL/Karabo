from karabo_gui.editablewidgets.floatspinbox import FloatSpinBox
from .base import BaseWidgetContainer


class _FloatSpinBoxWrapper(FloatSpinBox):
    """ A wrapper around FloatSpinBox to hooks into some methods.
    """

    def __init__(self, model, box, parent):
        super(_FloatSpinBoxWrapper, self).__init__(box, parent)
        self.model = model
        super(_FloatSpinBoxWrapper, self)._setStep(self.model.step)

    def _setStep(self, step):
        super(_FloatSpinBoxWrapper, self)._setStep(step)
        self.model.step = step


class FloatSpinBoxContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _FloatSpinBoxWrapper(self.model, boxes[0], self)
