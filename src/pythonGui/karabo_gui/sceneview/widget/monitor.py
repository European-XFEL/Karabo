from karabo_gui.displaywidgets.monitor import Monitor
from .base import BaseWidgetContainer


class _MonitorWrapper(Monitor):
    """ A wrapper around Monitor
    """
    def __init__(self, model, box, parent):
        super(_MonitorWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_MonitorWrapper, self)._setFilename(self.model.filename)
        super(_MonitorWrapper, self)._setInterval(self.model.interval)

    def _setFilename(self, filename):
        super(_MonitorWrapper, self)._setFilename(filename)
        self.model.filename = filename

    def _setInterval(self, interval):
        super(_MonitorWrapper, self)._setInterval(interval)
        self.model.interval = interval


class MonitorContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _MonitorWrapper(self.model, boxes[0], self)
