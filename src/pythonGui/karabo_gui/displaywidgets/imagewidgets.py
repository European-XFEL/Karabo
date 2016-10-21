#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple

from PyQt4.QtCore import QSize, Qt, pyqtSlot
from PyQt4.QtGui import (QAction, QCursor, QHBoxLayout, QImage, QMenu,
                         QToolButton, QVBoxLayout, QWidget)
from PyQt4.Qwt5.Qwt import QwtPlot

import karabo_gui.icons as icons
from guiqwt.builder import make
from guiqwt.plot import ImageWidget
from karabo_gui.images import get_dimensions_and_format, get_image_data
from karabo_gui.schema import ImageNode
from karabo_gui.widget import DisplayWidget

ToolInfo = namedtuple('ToolInfo', ('text', 'tooltip', 'icon'))


_TOOLBUTTON_INFO = OrderedDict()
_TOOLBUTTON_INFO['mouse_pointer'] = ToolInfo(
    text='Selection', tooltip='Selection Cursor', icon=icons.cursorArrow.icon)
_TOOLBUTTON_INFO['zoom'] = ToolInfo(
    text='Zoom', tooltip='Rectangle Zoom', icon=icons.zoom.icon)
_TOOLBUTTON_INFO['reset'] = ToolInfo(
    text='Reset', tooltip='Reset', icon=icons.maximize.icon)
_TOOLBUTTON_INFO['roi'] = ToolInfo(
    text='Region of Interest', tooltip='Show Region of Interest',
    icon=icons.crop.icon)


_CONTEXT_MENU_INFO = OrderedDict()
_CONTEXT_MENU_INFO['tool_bar'] = ToolInfo(
    text='Show tool bar', tooltip='Show tool bar for widget', icon=None)
_CONTEXT_MENU_INFO['color_bar'] = ToolInfo(
    text='Show color bar', tooltip='Show color bar for widget', icon=None)
_CONTEXT_MENU_INFO['axes'] = ToolInfo(
    text='Show axes', tooltip='Show axes for widget', icon=None)


class BaseImageDisplay(DisplayWidget):
    def __init__(self, box, parent):
        super(BaseImageDisplay, self).__init__(box)
        self._initUI(parent)

    def _initUI(self, parent):
        """ Initialize widget """
        self.image_widget = ImageWidget(parent=parent)
        self.toolbar_widget = self._create_toolbar()

        self.widget = QWidget()
        self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.widget.customContextMenuRequested.connect(self.show_context_menu)
        self.layout = QHBoxLayout(self.widget)
        self.layout.addWidget(self.image_widget)
        self.layout.addWidget(self.toolbar_widget)

        self.plot = self.image_widget.get_plot()

    def _create_toolbar(self):
        """ Toolbar gets created """
        toolbar_widget = QWidget()
        toolbar_layout = QVBoxLayout(toolbar_widget)
        toolbar_layout.setContentsMargins(0, 0, 0, 0)

        for key, info in _TOOLBUTTON_INFO.items():
            btn = self._create_toolbutton(info)
            btn.clicked.connect(getattr(self, "{}_clicked".format(key)))
            toolbar_layout.addWidget(btn)

        toolbar_layout.addStretch()
        return toolbar_widget

    def _create_toolbutton(self, btn_info):
        """ Create tool button from given ``btn_info`` which is of type
        ``ToolInfo`` """
        btn = QToolButton()
        btn.setToolTip(btn_info.tooltip)
        btn.setIconSize(QSize(24, 24))
        btn.setIcon(btn_info.icon)
        return btn

    def _create_action(self, action_info):
        """ Create action from given ``action_info`` which is of type
        ``ToolInfo`` """
        q_action = QAction(action_info.text, self)
        q_action.setCheckable(True)
        q_action.setStatusTip(action_info.tooltip)
        q_action.setToolTip(action_info.tooltip)
        return q_action

    def _show_tool_bar(self, show):
        self.toolbar_widget.setVisible(show)

    def _show_color_bar(self, show):
        self.plot.enableAxis(QwtPlot.yRight, show)

    def _show_axes(self, show):
        self.plot.enableAxis(QwtPlot.xBottom, show)
        self.plot.enableAxis(QwtPlot.yLeft, show)

    def valueChanged(self, box, value, timestamp=None):
        dimX, dimY, dimZ, format = get_dimensions_and_format(value)
        if dimX is None and dimY is None:
            return

        npy = get_image_data(value)
        if npy is None:
            return
        if format is QImage.Format_Indexed8:
            try:
                npy.shape = dimY, dimX
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise
        elif format is QImage.Format_RGB888:
            try:
                npy.shape = dimY, dimX, dimZ
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise
        else:
            try:
                npy.shape = dimY, dimX, dimZ
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise

        # Some dtypes (eg uint32) are not displayed -> astype('float')
        image = make.image(npy.astype('float'))
        self.plot.add_item(image)

        # In case an axis is disabled - the scale needs to be adapted
        img_rect = image.boundingRect()
        img_width = img_rect.width()
        img_height = img_rect.height()
        DELTA = 0.2
        if not self._axes_shown():
            self.plot.setAxisScale(
                QwtPlot.xBottom, img_rect.x() - DELTA, img_width + DELTA)
            self.plot.setAxisScale(
                QwtPlot.yLeft, img_rect.y() - DELTA, img_height + DELTA)

        self.plot.replot()

    @pyqtSlot(object)
    def show_context_menu(self, pos):
        """ The context menu is requested """
        menu = QMenu()
        for key, info in _CONTEXT_MENU_INFO.items():
            action = self._create_action(info)
            action.setChecked(getattr(self, "_{}_shown".format(key))())
            action.triggered.connect(getattr(self, "_show_{}".format(key)))
            menu.addAction(action)
        menu.exec(QCursor.pos())

    @pyqtSlot()
    def mouse_pointer_clicked(self):
        pass

    @pyqtSlot()
    def zoom_clicked(self):
        pass

    @pyqtSlot()
    def reset_clicked(self):
        pass

    @pyqtSlot()
    def roi_clicked(self):
        pass


class WebcamImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Webcam image"

    def __init__(self, box, parent):
        super(WebcamImageDisplay, self).__init__(box, parent)


class ScientificImageDisplay(BaseImageDisplay):
    category = ImageNode
    priority = 10
    alias = "Scientific image"

    def __init__(self, box, parent):
        super(ScientificImageDisplay, self).__init__(box, parent)
