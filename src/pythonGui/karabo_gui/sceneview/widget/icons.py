from karabo_gui.displaywidgets.icons import (
    DigitIcons, TextIcons, SelectionIcons)
from karabo_gui.scenemodel.api import (
    IconData, DigitIconsModel, SelectionIconsModel, TextIconsModel)
from .base import BaseWidgetContainer


class _IconsWrapperMixin(object):
    def __init__(self, model, box, parent):
        super(_IconsWrapperMixin, self).__init__(box, parent)
        self.model = model

    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        raise NotImplementedError

    def _setItems(self, items):
        self.model.values = [self.item_convert(i) for i in items]


class _DigitIconsWrapper(_IconsWrapperMixin, DigitIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=getattr(item, 'equal', False),
                        value=str(item.value),
                        image=item.url)


class _SelectionIconsWrapper(_IconsWrapperMixin, SelectionIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False, value=item.value, image=item.url)


class _TextIconsWrapper(_IconsWrapperMixin, TextIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False, value=item.value, image=item.url)


class IconsContainer(BaseWidgetContainer):
    """ A container for the various Icons display widgets.
    """
    def _create_widget(self, boxes):
        factories = {
            DigitIconsModel: _DigitIconsWrapper,
            SelectionIconsModel: _SelectionIconsWrapper,
            TextIconsModel: _TextIconsWrapper,
        }
        factory = factories[self.model.__class__]
        return factory(self.model, boxes[0], self)
