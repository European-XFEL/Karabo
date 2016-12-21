from collections import OrderedDict

from karabo_gui.displaywidgets.displaystatecolor import DisplayStateColor
from .base import BaseWidgetContainer


class _StateColorWrapper(DisplayStateColor):
    """ A wrapper around DisplayStateColor to hooks into some methods.
    """

    def __init__(self, model, box, parent):
        super(_StateColorWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_StateColorWrapper, self)._setStaticText(model.text)

    def _setStaticText(self, text):
        super(_StateColorWrapper, self)._setStaticText(text)
        self.model.text = text


class DisplayStateColor
    Container(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _StateColorWrapper(self.model, boxes[0], self)
