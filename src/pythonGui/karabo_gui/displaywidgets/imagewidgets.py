#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod
from contextlib import contextmanager
from functools import partial

from guiqwt.builder import make
from guiqwt.plot import ImageWidget
from guiqwt.tools import RectZoomTool, SelectTool

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QAction, QActionGroup, QCursor, QHBoxLayout, QIcon, QMenu, QToolBar,
    QWidget
)
from PyQt4.Qwt5.Qwt import QwtPlot

from traits.api import ABCHasStrictTraits, Bool, Callable, Instance, String

import karabo_gui.icons as icons
from karabo_gui.images import get_dimensions_and_format, get_image_data
from karabo_gui.schema import ImageNode
from karabo_gui.widget import DisplayWidget


class BaseImageDisplay(DisplayWidget):
    def __init__(self, box, parent):
        super(BaseImageDisplay, self).__init__(box)
        self._initUI(parent)

    def _initUI(self, parent):
        """ Initialize widget """
        self.image_widget = ImageWidget(parent=parent)
        default_tool = self.image_widget.add_tool(SelectTool)
        self.image_widget.set_default_tool(default_tool)
        self.image_widget.add_tool(RectZoomTool)
        self.set_tool(SelectTool)
        self.toolbar_widget = self._create_toolbar()

        self.widget = QWidget()
        self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.widget.customContextMenuRequested.connect(self.show_context_menu)
        self.layout = QHBoxLayout(self.widget)
        self.layout.addWidget(self.image_widget)
        self.layout.addWidget(self.toolbar_widget)

        self.plot = self.image_widget.get_plot()
        self.image = None  # actual image

    def _create_toolbar(self):
        """ Toolbar gets created """
        qactions = []

        toolbar_widget = QToolBar()
        toolbar_widget.setFloatable(False)
        toolbar_widget.setOrientation(Qt.Vertical)

        mode_qactions = [self._build_qaction(a)
                         for a in self._create_mode_tool_actions()]
        self._build_qaction_group(mode_qactions)
        qactions.extend(mode_qactions)

        qactions.extend(self._build_qaction(a)
                        for a in self._create_image_actions())

        for qaction in qactions:
            toolbar_widget.addAction(qaction)

        return toolbar_widget

    def _create_mode_tool_actions(self):
        """ Create actions and return list of them"""
        selection = WidgetSelectionAction(
            icon=icons.cursorArrow,
            checkable=True,
            is_checked=True,
            text="Selection Mode",
            tooltip="Selection Mode"
        )
        zoom = WidgetZoomAction(
            icon=icons.zoom,
            checkable=True,
            is_checked=False,
            text="Zoom",
            tooltip="Rectangle Zoom"
        )

        return [selection, zoom]

    def _create_image_actions(self):
        """ Create actions and return list of them"""
        roi = WidgetROIAction(
            icon=icons.crop,
            checkable=True,
            is_checked=False,
            text="Region of Interest",
            tooltip="Show Region of Interest"
        )

        return [roi]

    def _create_context_menu_actions(self):
        actions = []
        actions.append(ShowToolBarAction(
            text="Show tool bar",
            tooltip="Show tool bar for widget",
            checkable=True,
            is_checked=self._tool_bar_shown())
        )
        actions.append(ShowColorBarAction(
            text="Show color bar",
            tooltip="Show color bar for widget",
            checkable=True,
            is_checked=self._color_bar_shown())
        )
        actions.append(ShowAxesAction(
            text="Show axes",
            tooltip="Show axes for widget",
            checkable=True,
            is_checked=self._axes_shown())
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
        q_action.triggered.connect(partial(widget_action.perform, self))
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
        tool_obj = self.image_widget.get_tool(ToolKlass)
        if tool_obj is not None:
            # Use guiqwt.tools method
            tool_obj.activate()
        self.current_tool_obj = tool_obj

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
        img_rect = self.image.boundingRect()
        img_width = img_rect.width()
        img_height = img_rect.height()
        DELTA = 0.2
        # XXX TODO: refer to zoom rectangle once this is set
        if not isinstance(self.current_tool_obj, RectZoomTool):
            self.plot.setAxisScale(
                QwtPlot.xBottom, img_rect.x() - DELTA, img_width + DELTA)
            self.plot.setAxisScale(
                QwtPlot.yLeft, img_rect.y() - DELTA, img_height + DELTA)

        self.plot.replot()

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
        tool_obj.deactivate()
        yield
        tool_obj.activate()


class WebcamImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Webcam image"


class ScientificImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Scientific image"


class BaseWidgetAction(ABCHasStrictTraits):
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
    triggered = Callable('perform')


class WidgetSelectionAction(BaseWidgetAction):
    """ Selection action"""
    def perform(self, widget):
        widget.set_tool(SelectTool)


class WidgetZoomAction(BaseWidgetAction):
    """ Selection action"""
    def perform(self, widget):
        widget.set_tool(RectZoomTool)


class WidgetROIAction(BaseWidgetAction):
    """ Region of interest action"""
    def perform(self, widget):
        pass


class ShowToolBarAction(BaseWidgetAction):
    """ Show tool bar action"""
    def perform(self, widget, is_checked):
        self.is_checked = is_checked
        widget._show_tool_bar(is_checked)


class ShowColorBarAction(BaseWidgetAction):
    """ Show color bar action"""
    def perform(self, widget, is_checked):
        self.is_checked = is_checked
        widget._show_color_bar(is_checked)


class ShowAxesAction(BaseWidgetAction):
    """ Show axes action"""
    def perform(self, widget, is_checked):
        self.is_checked = is_checked
        widget._show_axes(is_checked)
