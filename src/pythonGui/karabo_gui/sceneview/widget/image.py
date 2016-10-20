#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo.common.scenemodel.api import (DisplayAlignedImageModel,
                                          DisplayImageElementModel,
                                          DisplayImageModel,
                                          ScientificImageModel,
                                          WebcamImageModel)
from karabo_gui.displaywidgets.imagewidgets import (ScientificImageDisplay,
                                                    WebcamImageDisplay)
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
        super(_ComplexImageWrapperMixin, self)._showToolBar(
            model.show_tool_bar)
        super(_ComplexImageWrapperMixin, self)._showColorBar(
            model.show_color_bar)
        super(_ComplexImageWrapperMixin, self)._showAxes(model.show_axes)

    def _showToolBar(self, show):
        super(_ComplexImageWrapperMixin, self)._showToolBar(show)
        self.model.show_tool_bar = show

    def _showColorBar(self, show):
        super(_ComplexImageWrapperMixin, self)._showColorBar(show)
        self.model.show_color_bar = show

    def _showAxes(self, show):
        super(_ComplexImageWrapperMixin, self)._showAxes(show)
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
