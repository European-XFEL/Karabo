from karabo_gui.displaywidgets.displaycolorbool import DisplayColorBool
from .base import BaseWidgetContainer


class _ColorBoolWrapper(DisplayColorBool):
    """ A wrapper around DisplayColorBool to hooks into some methods.
    """

    def __init__(self, model, box, parent):
        super(_ColorBoolWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_ColorBoolWrapper, self)._setInvert(model.invert)

    def _setInvert(self, value):
        super(_ColorBoolWrapper, self)._setInvert(value)
        self.model.invert = value


class ColorBoolContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _ColorBoolWrapper(self.model, boxes[0], self)
