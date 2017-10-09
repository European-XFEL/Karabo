from karabo_gui.editablewidgets.floatspinbox import FloatSpinBox
from .base import BaseWidgetContainer


class _FloatSpinBoxWrapper(FloatSpinBox):
    """ A wrapper around FloatSpinBox to hooks into some methods.
    """

    def __init__(self, model, box, parent):
        super(_FloatSpinBoxWrapper, self).__init__(box, parent)
        self.model = model
        super(_FloatSpinBoxWrapper, self)._set_step(self.model.step)
        super(_FloatSpinBoxWrapper, self)._set_decimals(self.model.decimals)

    def _set_step(self, step):
        super(_FloatSpinBoxWrapper, self)._set_step(step)
        self.model.step = step

    def _set_decimals(self, decimals):
        super(_FloatSpinBoxWrapper, self)._set_decimals(decimals)
        self.model.decimals = decimals


class FloatSpinBoxContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _FloatSpinBoxWrapper(self.model, boxes[0], self)
