from karabo_gui.displaywidgets.eval import Evaluator
from .base import BaseWidgetContainer


class _EvaluatorWrapper(Evaluator):
    """ A wrapper around Evaluator
    """
    def __init__(self, model, box, parent):
        super(_EvaluatorWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_EvaluatorWrapper, self).setText(model.expression)

    def setText(self, text):
        super(_EvaluatorWrapper, self).setText(text)
        self.model.expression = text


class EvaluatorContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        display_widget = _EvaluatorWrapper(self.model, boxes[0], self)
        return display_widget.widget
