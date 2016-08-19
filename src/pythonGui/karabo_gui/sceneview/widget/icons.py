from karabo.common.scenemodel.api import (
    IconData, DigitIconsModel, SelectionIconsModel, TextIconsModel)
from karabo_gui.displaywidgets.icons import (
    DigitIcons, Item, TextIcons, SelectionIcons)
from .base import BaseWidgetContainer


class _IconsWrapperMixin(object):
    def __init__(self, model, box, parent):
        super(_IconsWrapperMixin, self).__init__(box, parent)
        self.model = model
        items = []
        for icon_data in self.model.values:
            item = Item(icon_data.value, icon_data.data)
            items.append(item)
        self._setItems(items)

    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        raise NotImplementedError

    def _setItems(self, items):
        self.model.values = [self.item_convert(i) for i in items]
        super(_IconsWrapperMixin, self)._setItems(items)


class _DigitIconsWrapper(_IconsWrapperMixin, DigitIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=getattr(item, 'equal', False),
                        value=str(item.value),
                        image=item.url or "",
                        data=item.data or "")


class _SelectionIconsWrapper(_IconsWrapperMixin, SelectionIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False,
                        value=item.value,
                        image=item.url or "",
                        data=item.data or "")


class _TextIconsWrapper(_IconsWrapperMixin, TextIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False,
                        value=item.value,
                        image=item.url or "",
                        data=item.data or "")


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
