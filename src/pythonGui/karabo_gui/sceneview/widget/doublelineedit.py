from karabo_gui.editablewidgets.numberlineedit import DoubleLineEdit
from .base import BaseWidgetContainer


class _DoubleLineEditWrapper(DoubleLineEdit):
    """ A wrapper around DoubleLineEdit to hooks into some methods.
    """

    def __init__(self, model, box, parent):
        super(_DoubleLineEditWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_DoubleLineEditWrapper, self)._setDecimalNumber(model.decimals)

    def _setDecimalNumber(self, value):
        super(_DoubleLineEditWrapper, self)._setDecimalNumber(value)
        self.model.decimals = value


class DoubleLineEditContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _DoubleLineEditWrapper(self.model, boxes[0], self)
