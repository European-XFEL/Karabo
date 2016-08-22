from karabo.common.scenemodel.api import (
    DigitIconsModel, IconData, SelectionIconsModel, TextIconsModel)
from karabo_gui.displaywidgets.displayiconset import DisplayIconset
from karabo_gui.displaywidgets.icons import (
    DigitIcons, Item, SelectionIcons, TextIcons)
from .base import BaseWidgetContainer


class _IconsWrapperMixin(object):
    def __init__(self, model, box, parent):
        super(_IconsWrapperMixin, self).__init__(box, parent)
        self.model = model
        self.items = [Item(m.value, m.data) for m in self.model.values]

    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        raise NotImplementedError

    def _setItems(self, items):
        super(_IconsWrapperMixin, self)._setItems(items)
        self.model.values = [self.item_convert(i) for i in items]


class _DigitIconsWrapper(_IconsWrapperMixin, DigitIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=getattr(item, 'equal', False),
                        value=str(item.value),
                        image=item.url or "",
                        data=item.data)


class _SelectionIconsWrapper(_IconsWrapperMixin, SelectionIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False,
                        value=item.value,
                        image=item.url or "",
                        data=item.data)


class _TextIconsWrapper(_IconsWrapperMixin, TextIcons):
    def item_convert(self, item):
        """ Convert a single item to an ItemData """
        return IconData(equal=False,
                        value=item.value,
                        image=item.url or "",
                        data=item.data)


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


class _DisplayIconsetWrapper(DisplayIconset):
    """ A wrapper around Evaluator
    """
    def __init__(self, model, box, parent):
        self.model = model  # Needs to be set here because `setURL` is already
                            # called in the widgets constructor
        super(_DisplayIconsetWrapper, self).__init__(box, parent)
        # Initialize the widget
        super(_DisplayIconsetWrapper, self).setData(model.image, model.data)

    def setURL(self, image):
        super(_DisplayIconsetWrapper, self).setURL(image)
        self.model.image = image

    def setData(self, image, data):
        super(_DisplayIconsetWrapper, self).setData(data)
        self.model.image = image
        self.model.data = data


class DisplayIconsetContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _DisplayIconsetWrapper(self.model, boxes[0], self)
