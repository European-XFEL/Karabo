from karabo_gui.displaywidgets.displaysparkline import DisplaySparkline
from .base import BaseWidgetContainer


class SparklineContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return DisplaySparkline(self.model, boxes[0], self)
