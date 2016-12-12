from karabo_gui.displaywidgets.statefuliconwidget import StatefulIconWidget
from karabo_gui.icons.statefulicons import ICONS
from .base import BaseWidgetContainer


class _StatefulIconWrapper(StatefulIconWidget):

    def __init__(self, model, box, parent):
        icon = ICONS.get(model.icon_name, None)
        self.model = model
        super(_StatefulIconWrapper, self).__init__(box, icon, parent)

    def _setIcon(self, icon):
        super(_StatefulIconWrapper, self)._setIcon(icon)
        self.model.icon_name = icon.name


class StatefulIconContainer(BaseWidgetContainer):
    """ A container for StatefulIcons
    """
    def _create_widget(self, boxes):
        return _StatefulIconWrapper(self.model, boxes[0], self)
