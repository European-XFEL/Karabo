#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
from functools import partial

from guiqwt.builder import make
from guiqwt.tools import RectZoomTool, SelectTool
from PyQt4.QtCore import pyqtSlot, QRect, Qt
from PyQt4.QtGui import (
    QAction, QActionGroup, QCursor, QHBoxLayout, QIcon, QMenu, QToolBar,
    QWidget)
from PyQt4.Qwt5.Qwt import QwtPlot
from traits.api import (
    Bool, Callable, HasStrictTraits, Instance, on_trait_change, String)

from karabo.common.scenemodel.api import ScientificImageModel, WebcamImageModel
from karabogui import icons
from karabogui.binding.api import ImageBinding
from karabogui.controllers.images import (
    get_dimensions_and_format, get_image_data, KaraboImageWidget)
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.util import SignalBlocker


class _BaseImage(BaseBindingController):
    _plot = Instance(object)  # qwt class
    _current_tool = Instance(object)  # qwt class

    _image_widget = Instance(KaraboImageWidget)
    _zoom_rect = Instance(QRect)
    _toolbar = Instance(QToolBar)

    def create_widget(self, parent):
        """ Initialize widget """
        self._image_widget = KaraboImageWidget(parent=parent)
        # Make connection to keep zoom_rect in sync
        self._image_widget.signal_mouse_event.connect(self.zoom_reset)
        self._plot = self._image_widget.get_plot()

        # Add tools
        self._current_tool = None
        default_tool = self._image_widget.add_tool(SelectTool)
        self._image_widget.set_default_tool(default_tool)
        self._image_widget.add_tool(RectZoomTool)
        self._zoom_rect = QRect()
        self._set_tool(SelectTool)
        self._toolbar = self._create_toolbar()

        widget = QWidget()
        widget.setContextMenuPolicy(Qt.CustomContextMenu)
        widget.customContextMenuRequested.connect(self.show_context_menu)
        hlayout = QHBoxLayout(widget)
        hlayout.addWidget(self._image_widget)
        hlayout.addWidget(self._toolbar)
        return widget

    @on_trait_change('proxies.binding.config_update')
    def _update_image(self, obj, name, value):
        if name != 'config_update':
            return
        if self.widget is None:
            return

        img_node = obj.value

        dimX, dimY, dimZ, format = get_dimensions_and_format(img_node)
        if dimX is None and dimY is None:
            return

        npy = get_image_data(img_node, dimX, dimY, dimZ, format)
        if npy is None:
            return

        # Check for already existing image items
        image_items = self._image_widget.get_image_item()
        if not image_items:
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            image_item = make.image(npy.astype('float'))
            self._plot.add_item(image_item)
        else:
            # Manipulate top item
            last_img = image_items[-1]
            lut_range = last_img.get_lut_range()
            if lut_range[0] == lut_range[1]:
                # if not set not set, let it autoscale
                lut_range = None
            last_img.set_data(npy.astype('float'), lut_range=lut_range)

            # In case an axis is disabled - the scale needs to be adapted
            if self._zoom_rect.isEmpty():
                DELTA = 0.2
                img_rect = last_img.boundingRect()
                img_width = img_rect.width()
                img_height = img_rect.height()
                x_axis = self._plot.axisWidget(QwtPlot.xBottom)
                with SignalBlocker(x_axis):
                    self._plot.setAxisScale(
                        QwtPlot.xBottom,
                        img_rect.x() - DELTA, img_width + DELTA)
                y_axis = self._plot.axisWidget(QwtPlot.yLeft)
                with SignalBlocker(y_axis):
                    # Note: y axis is shown in reverse order
                    # (max - lowerBound, min - upperBound)
                    self._plot.setAxisScale(
                        QwtPlot.yLeft,
                        img_height + DELTA, img_rect.y() - DELTA)
        self._plot.replot()

    # -----------------------------------------------------------------------
    # private functions

    def _create_toolbar(self):
        """ Toolbar gets created """
        qactions = []

        toolbar_widget = QToolBar()
        toolbar_widget.setFloatable(False)
        toolbar_widget.setOrientation(Qt.Vertical)

        mode_qactions = self._create_mode_tool_actions()
        self._build_qaction_group(mode_qactions)
        qactions.extend(mode_qactions)

        for qaction in qactions:
            toolbar_widget.addAction(qaction)

        return toolbar_widget

    def _create_mode_tool_actions(self):
        """ Create actions and return list of them"""
        selection = WidgetAction(
            icon=icons.cursorArrow,
            checkable=True,
            is_checked=True,
            text="Selection Mode",
            tooltip="Selection Mode",
            triggered=partial(self._tool_handler, SelectTool)
        )
        zoom = WidgetAction(
            icon=icons.zoom,
            checkable=True,
            is_checked=False,
            text="Zoom",
            tooltip="Rectangle Zoom",
            triggered=partial(self._tool_handler, RectZoomTool)
        )

        return [self._build_qaction(a, build=False) for a in (selection, zoom)]

    def _create_context_menu_actions(self):
        actions = []
        actions.append(WidgetAction(
            text="Show tool bar",
            tooltip="Show tool bar for widget",
            checkable=True,
            is_checked=self.model.show_tool_bar,
            triggered=show_toolbar_handler)
        )
        actions.append(WidgetAction(
            text="Show color bar",
            tooltip="Show color bar for widget",
            checkable=True,
            is_checked=self.model.show_color_bar,
            triggered=show_colorbar_handler)
        )
        actions.append(WidgetAction(
            text="Show axes",
            tooltip="Show axes for widget",
            checkable=True,
            is_checked=self.model.show_axes,
            triggered=show_axes_handler)
        )
        q_actions = [self._build_qaction(a) for a in actions]
        return q_actions

    def _build_qaction(self, widget_action, build=True):
        """ Create action from given ``widget_action`` """
        q_action = QAction(widget_action.text, self.widget)
        if widget_action.icon is not None:
            q_action.setIcon(widget_action.icon)
        q_action.setCheckable(widget_action.checkable)
        if widget_action.checkable:
            q_action.setChecked(widget_action.is_checked)
        q_action.setStatusTip(widget_action.tooltip)
        q_action.setToolTip(widget_action.tooltip)
        callback = partial(widget_action.triggered,
                           widget_action,
                           self.model)
        q_action.toggled.connect(callback)
        return q_action

    def _build_qaction_group(self, actions):
        group = QActionGroup(self.widget)
        for ac in actions:
            group.addAction(ac)
        actions[0].setChecked(True)  # Mark the first action

    def _show_tool_bar(self, show):
        self._toolbar.setVisible(show)

    # -----------------------------------------------------------------------
    # qwt related

    def _show_color_bar(self, show):
        self._plot.enableAxis(QwtPlot.yRight, show)

    def _show_axes(self, show):
        self._plot.enableAxis(QwtPlot.xBottom, show)
        self._plot.enableAxis(QwtPlot.yLeft, show)

    def _set_tool(self, ToolKlass):
        """ Sets the current tool being used by the widget.

        :param ToolKlass: Describes the guiqwt.tools class
        """
        if isinstance(self._current_tool, RectZoomTool):
            self._plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.disconnect(
                self.axis_changed)
            self._plot.axisWidget(QwtPlot.yLeft).scaleDivChanged.disconnect(
                self.axis_changed)
        tool_obj = self._image_widget.get_tool(ToolKlass)
        if tool_obj is not None:
            # Use guiqwt.tools method
            tool_obj.activate()
        self._current_tool = tool_obj
        if isinstance(self._current_tool, RectZoomTool):
            self._plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.connect(
                self.axis_changed)
            self._plot.axisWidget(QwtPlot.yLeft).scaleDivChanged.connect(
                self.axis_changed)

    def _tool_handler(self, tool, *args):
        # use *args to match signature for partial in _build_qaction
        self._set_tool(tool)

    # -----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def zoom_reset(self):
        self._zoom_rect = QRect()

    @pyqtSlot()
    def axis_changed(self):
        x_axis = self._plot.axisScaleDiv(QwtPlot.xBottom)
        x1, x2 = x_axis.lowerBound(), x_axis.upperBound()
        y_axis = self._plot.axisScaleDiv(QwtPlot.yLeft)
        # Note: y axis is shown in reverse order
        # (max - lowerBound, min - upperBound)
        y1, y2 = y_axis.lowerBound(), y_axis.upperBound()
        self._zoom_rect = QRect(x1, y1, x2 - x1, y1 - y2)

    @pyqtSlot(object)
    def show_context_menu(self, pos):
        """ The context menu is requested """
        with keep_tool_activated(self._current_tool):
            menu = QMenu()
            q_actions = self._create_context_menu_actions()
            for a in q_actions:
                menu.addAction(a)
            menu.exec(QCursor.pos())


# XXX: WebcamImageDisplay and ScientificImageDisplay are identical on every
# level, we should remove one of them!
# XXX: priority = 10
@register_binding_controller(ui_name='Webcam Image', binding_type=ImageBinding)
class WebcamImageDisplay(_BaseImage):
    model = Instance(WebcamImageModel)


# XXX: priority = 10
@register_binding_controller(ui_name='Scientific Image',
                             binding_type=ImageBinding)
class ScientificImageDisplay(_BaseImage):
    model = Instance(ScientificImageModel)


class WidgetAction(HasStrictTraits):
    """ Base class for actions in a widget
    """
    # The icon for this action
    icon = Instance(QIcon)
    # The text label for the action
    text = String
    # The tooltip text shown when hovering over the action
    tooltip = String
    # Whether or not this action is checkable
    checkable = Bool(False)
    # Whether or not this action is checked
    is_checked = Bool(False)
    # Defines the method which is called whenever the action is triggered
    triggered = Callable


def show_toolbar_handler(action, model, is_checked):
    model.show_tool_bar = is_checked
    action.is_checked = is_checked


def show_colorbar_handler(action, model, is_checked):
    model.show_color_bar = is_checked
    action.is_checked = is_checked


def show_axes_handler(action, model, is_checked):
    model.show_axes = is_checked
    action.is_checked = is_checked


@contextmanager
def keep_tool_activated(tool_obj):
    """ Context manager to keep tool activated - needs to be done to avoid
    some strange behaviour with the ``guiqwt.ImageWidget``

    :param tool_obj: An object of type ``guiqwt.tools``
    """
    try:
        tool_obj.deactivate()
        yield
    finally:
        tool_obj.activate()
