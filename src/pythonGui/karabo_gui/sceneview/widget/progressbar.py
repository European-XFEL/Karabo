from karabo_gui.displaywidgets.displayprogressbar import DisplayProgressBar
from .base import BaseWidgetContainer


class ProgressBarContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return DisplayProgressBar(self.model, boxes[0], self)
