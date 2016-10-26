#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
from functools import partial

from guiqwt.builder import make
from guiqwt.plot import ImageWidget
from guiqwt.tools import RectZoomTool, SelectTool

from PyQt4.QtCore import pyqtSignal, pyqtSlot, QRect, Qt
from PyQt4.QtGui import (
    QAction, QActionGroup, QCursor, QHBoxLayout, QIcon, QMenu, QToolBar,
    QWidget
)
from PyQt4.Qwt5.Qwt import QwtPlot

from traits.api import HasStrictTraits, Bool, Callable, Instance, String

import karabo_gui.icons as icons
from karabo_gui.images import get_dimensions_and_format, get_image_data
from karabo_gui.schema import ImageNode
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class _KaraboImageWidget(ImageWidget):
    signal_mouse_event = pyqtSignal()

    def __init__(self, **kwargs):
        """ Possible key arguments:
            * parent: parent widget
            * title: plot title (string)
            * xlabel, ylabel, zlabel: resp. bottom, left and right axis titles
            (strings)
            * xunit, yunit, zunit: resp. bottom, left and right axis units
            (strings)
            * yreverse: reversing Y-axis (bool)
            * aspect_ratio: height to width ratio (float)
            * lock_aspect_ratio: locking aspect ratio (bool)
            * show_contrast: showing contrast adjustment tool (bool)
            * show_xsection: showing x-axis cross section plot (bool)
            * show_ysection: showing y-axis cross section plot (bool)
            * xsection_pos: x-axis cross section plot position
            (string: "top", "bottom")
            * ysection_pos: y-axis cross section plot position
            (string: "left", "right")
            * panels (optional): additionnal panels (list, tuple)
        """
        super(_KaraboImageWidget, self).__init__(**kwargs)

    def mousePressEvent(self, event):
        """ Overwrite method to forward middle button press event which
        resets the image """
        super(_KaraboImageWidget, self).mousePressEvent(event)
        if event.button() is Qt.MidButton:
            self.signal_mouse_event.emit()


class BaseImageDisplay(DisplayWidget):
    def __init__(self, box, parent):
        super(BaseImageDisplay, self).__init__(box)
        self._initUI(parent)

    def _initUI(self, parent):
        """ Initialize widget """
        self.image_widget = _KaraboImageWidget(parent=parent)
        # Make connection to keep zoom_rect in sync
        self.image_widget.signal_mouse_event.connect(self.zoom_reset)
        self.plot = self.image_widget.get_plot()
        self.image = None  # actual image

        # Add tools
        self.current_tool_obj = None
        default_tool = self.image_widget.add_tool(SelectTool)
        self.image_widget.set_default_tool(default_tool)
        self.image_widget.add_tool(RectZoomTool)
        self.zoom_rect = QRect()
        self.set_tool(SelectTool)
        self.toolbar_widget = self._create_toolbar()

        self.widget = QWidget()
        self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.widget.customContextMenuRequested.connect(self.show_context_menu)
        self.layout = QHBoxLayout(self.widget)
        self.layout.addWidget(self.image_widget)
        self.layout.addWidget(self.toolbar_widget)

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
            triggered=widget_selection_handler,
        )
        zoom = WidgetAction(
            icon=icons.zoom,
            checkable=True,
            is_checked=False,
            text="Zoom",
            tooltip="Rectangle Zoom",
            triggered=widget_zoom_handler,
        )

        return [self._build_qaction(a) for a in (selection, zoom)]

    def _create_context_menu_actions(self):
        actions = []
        actions.append(WidgetAction(
            text="Show tool bar",
            tooltip="Show tool bar for widget",
            checkable=True,
            is_checked=self._tool_bar_shown(),
            triggered=show_toolbar_handler)
        )
        actions.append(WidgetAction(
            text="Show color bar",
            tooltip="Show color bar for widget",
            checkable=True,
            is_checked=self._color_bar_shown(),
            triggered=show_colorbar_handler)
        )
        actions.append(WidgetAction(
            text="Show axes",
            tooltip="Show axes for widget",
            checkable=True,
            is_checked=self._axes_shown(),
            triggered=show_axes_handler)
        )
        q_actions = [self._build_qaction(a) for a in actions]
        return q_actions

    def _build_qaction(self, widget_action):
        """ Create action from given ``widget_action`` """
        q_action = QAction(widget_action.text, self)
        if widget_action.icon is not None:
            q_action.setIcon(widget_action.icon)
        q_action.setCheckable(widget_action.checkable)
        if widget_action.checkable:
            q_action.setChecked(widget_action.is_checked)
        q_action.setStatusTip(widget_action.tooltip)
        q_action.setToolTip(widget_action.tooltip)
        callback = partial(widget_action.triggered, widget_action, self)
        q_action.triggered.connect(callback)
        return q_action

    def _build_qaction_group(self, actions):
        group = QActionGroup(self)
        for ac in actions:
            group.addAction(ac)
        actions[0].setChecked(True)  # Mark the first action

    def _show_tool_bar(self, show):
        self.toolbar_widget.setVisible(show)

    def _show_color_bar(self, show):
        self.plot.enableAxis(QwtPlot.yRight, show)

    def _show_axes(self, show):
        self.plot.enableAxis(QwtPlot.xBottom, show)
        self.plot.enableAxis(QwtPlot.yLeft, show)

    def set_tool(self, ToolKlass):
        """ Sets the current tool being used by the widget.

        :param ToolKlass: Describes the guiqwt.tools class
        """
        if isinstance(self.current_tool_obj, RectZoomTool):
            self.plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.disconnect(
                self.axis_changed)
            self.plot.axisWidget(QwtPlot.yLeft).scaleDivChanged.disconnect(
                self.axis_changed)
        tool_obj = self.image_widget.get_tool(ToolKlass)
        if tool_obj is not None:
            # Use guiqwt.tools method
            tool_obj.activate()
        self.current_tool_obj = tool_obj
        if isinstance(self.current_tool_obj, RectZoomTool):
            self.plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.connect(
                self.axis_changed)
            self.plot.axisWidget(QwtPlot.yLeft).scaleDivChanged.connect(
                self.axis_changed)

    def valueChanged(self, box, value, timestamp=None):
        dimX, dimY, dimZ, format = get_dimensions_and_format(value)
        if dimX is None and dimY is None:
            return

        npy = get_image_data(value, dimX, dimY, dimZ, format)
        if npy is None:
            return

        if self.image is None:
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            self.image = make.image(npy.astype('float'))
            self.plot.add_item(self.image)
        else:
            self.image.set_data(npy.astype('float'))

        # In case an axis is disabled - the scale needs to be adapted
        if self.zoom_rect.isEmpty():
            DELTA = 0.2
            img_rect = self.image.boundingRect()
            img_width = img_rect.width()
            img_height = img_rect.height()
            x_axis = self.plot.axisWidget(QwtPlot.xBottom)
            with SignalBlocker(x_axis):
                self.plot.setAxisScale(
                    QwtPlot.xBottom, img_rect.x() - DELTA, img_width + DELTA)
            y_axis = self.plot.axisWidget(QwtPlot.yLeft)
            with SignalBlocker(y_axis):
                # Note: y axis is shown in reverse order
                # (max - lowerBound, min - upperBound)
                self.plot.setAxisScale(
                    QwtPlot.yLeft, img_height + DELTA, img_rect.y() - DELTA)
        self.plot.replot()

    @pyqtSlot()
    def zoom_reset(self):
        self.zoom_rect = QRect()

    @pyqtSlot()
    def axis_changed(self):
        x_axis = self.plot.axisScaleDiv(QwtPlot.xBottom)
        x1, x2 = x_axis.lowerBound(), x_axis.upperBound()
        y_axis = self.plot.axisScaleDiv(QwtPlot.yLeft)
        # Note: y axis is shown in reverse order (max - lowerBound,
        # min - upperBound)
        y1, y2 = y_axis.lowerBound(), y_axis.upperBound()
        self.zoom_rect = QRect(x1, y1, x2 - x1, y1 - y2)

    @pyqtSlot(object)
    def show_context_menu(self, pos):
        """ The context menu is requested """
        with self.keep_tool_activated(self.current_tool_obj):
            menu = QMenu()
            q_actions = self._create_context_menu_actions()
            for a in q_actions:
                menu.addAction(a)
            menu.exec(QCursor.pos())

    @contextmanager
    def keep_tool_activated(self, tool_obj):
        """ Context manager to keep tool activated - needs to be done to avoid
        some strange behaviour with the ``guiqwt.ImageWidget``

        :param tool_obj: An object of type ``guiqwt.tools``
        """
        try:
            tool_obj.deactivate()
            yield
        finally:
            tool_obj.activate()


class WebcamImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Webcam image"


class ScientificImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Scientific image"


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


def widget_selection_handler(action, widget):
    widget.set_tool(SelectTool)


def widget_zoom_handler(action, widget):
    widget.set_tool(RectZoomTool)


def show_toolbar_handler(action, widget, is_checked):
    action.is_checked = is_checked
    widget._show_tool_bar(is_checked)


def show_colorbar_handler(action, widget, is_checked):
    action.is_checked = is_checked
    widget._show_color_bar(is_checked)


def show_axes_handler(action, widget, is_checked):
    action.is_checked = is_checked
    widget._show_axes(is_checked)
