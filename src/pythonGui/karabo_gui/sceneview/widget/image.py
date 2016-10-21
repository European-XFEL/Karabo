#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo.common.scenemodel.api import (
    DisplayAlignedImageModel, DisplayImageElementModel, DisplayImageModel,
    ScientificImageModel, WebcamImageModel)
from karabo_gui.displaywidgets.imagewidgets import (
    ScientificImageDisplay, WebcamImageDisplay)
from karabo_gui.widget import Widget

from .base import BaseWidgetContainer

_SIMPLE_IMAGE_WIDGET_FACTORIES = {
    DisplayAlignedImageModel: 'DisplayAlignedImage',
    DisplayImageModel: 'DisplayImage',
    DisplayImageElementModel: 'DisplayImageElement',
}
_SIMPLE_IMAGE_WIDGET_FACTORIES = {
    k: Widget.widgets[v]
    for k, v in _SIMPLE_IMAGE_WIDGET_FACTORIES.items()}


class SimpleImageWidgetContainer(BaseWidgetContainer):
    """ A container for simple image scene widgets which have no additional
        model data which needs to be synchronized.
    """
    def _create_widget(self, boxes):
        factory = _SIMPLE_IMAGE_WIDGET_FACTORIES[self.model.__class__]
        widget = factory(boxes[0], self)
        for b in boxes[1:]:
            widget.addBox(b)
        return widget


class _ComplexImageWrapperMixin(object):
    def __init__(self, model, box, parent):
        super(_ComplexImageWrapperMixin, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_ComplexImageWrapperMixin, self)._show_tool_bar(
            model.show_tool_bar)
        super(_ComplexImageWrapperMixin, self)._show_color_bar(
            model.show_color_bar)
        super(_ComplexImageWrapperMixin, self)._show_axes(model.show_axes)

    def _tool_bar_shown(self):
        return self.model.show_tool_bar

    def _color_bar_shown(self):
        return self.model.show_color_bar

    def _axes_shown(self):
        return self.model.show_axes

    def _show_tool_bar(self, show):
        super(_ComplexImageWrapperMixin, self)._show_tool_bar(show)
        self.model.show_tool_bar = show

    def _show_color_bar(self, show):
        super(_ComplexImageWrapperMixin, self)._show_color_bar(show)
        self.model.show_color_bar = show

    def _show_axes(self, show):
        super(_ComplexImageWrapperMixin, self)._show_axes(show)
        self.model.show_axes = show


class _ScientificImageWrapper(_ComplexImageWrapperMixin,
                              ScientificImageDisplay):
    """ A wrapper around ScientificImageDisplay
    """


class _WebcamImageWrapper(_ComplexImageWrapperMixin, WebcamImageDisplay):
    """ A wrapper around WebcamImageDisplay
    """


class ComplexImageWidgetContainer(BaseWidgetContainer):
    """ A container for complex image scene widgets which have no additional
        model data which needs to be synchronized.
    """
    def _create_widget(self, boxes):
        factories = {
            ScientificImageModel: _ScientificImageWrapper,
            WebcamImageModel: _WebcamImageWrapper,
        }
        factory = factories[self.model.__class__]
        return factory(self.model, boxes[0], self)
