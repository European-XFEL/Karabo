#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple

from PyQt4.QtCore import QSize, pyqtSlot
from PyQt4.QtGui import QHBoxLayout, QImage, QToolButton, QVBoxLayout, QWidget
from PyQt4.Qwt5.Qwt import QwtPlot

import karabo_gui.icons as icons
from guiqwt.builder import make
from guiqwt.plot import ImageWidget
from karabo_gui.images import get_dimensions_and_format, get_image_data
from karabo_gui.schema import ImageNode
from karabo_gui.widget import DisplayWidget

ToolButtonInfo = namedtuple('ToolButtonInfo', ('tooltip', 'icon'))


_TOOLBUTTON_INFO = OrderedDict()
_TOOLBUTTON_INFO['mouse_pointer'] = ToolButtonInfo(
    tooltip='Mouse Pointer', icon=icons.cursorArrow.icon)
_TOOLBUTTON_INFO['zoom'] = ToolButtonInfo(tooltip='Zoom', icon=icons.zoom.icon)
_TOOLBUTTON_INFO['reset'] = ToolButtonInfo(
    tooltip='Reset', icon=icons.maximize.icon)
_TOOLBUTTON_INFO['roi'] = ToolButtonInfo(
    tooltip='Region of Interest', icon=icons.crop.icon)


class BaseImageDisplay(DisplayWidget):
    def __init__(self, box, parent):
        super(BaseImageDisplay, self).__init__(box)
        self._initUI(parent)

    def _initUI(self, parent):
        """ Initialize widget """
        self.image_widget = ImageWidget(parent=parent)
        self.toolbar_widget = self._create_toolbar()

        self.widget = QWidget()
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
        btn = QToolButton()
        btn.setToolTip(btn_info.tooltip)
        btn.setIconSize(QSize(24, 24))
        btn.setIcon(btn_info.icon)
        return btn

    def enableAxis(self, axisId, enable):
        """ Enable or disable the given ``axisId`` """
        self.plot.enableAxis(axisId, enable)

    def axisEnabled(self, axisId):
        # XXX: TODO refer to data model
        return self.plot.axisEnabled(axisId)

    def _showToolBar(self, show):
        print("_showToolBar", show)
        self.toolbar_widget.setVisible(show)

    def _showColorBar(self, show):
        print("_showColorBar", show)
        self.plot.enableAxis(QwtPlot.yRight, show)

    def _showAxes(self, show):
        print("_showAxes", show)
        self.enableAxis(QwtPlot.xBottom, show)
        self.enableAxis(QwtPlot.yLeft, show)

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
        if not self.axisEnabled(QwtPlot.xBottom):
            self.plot.setAxisScale(
                QwtPlot.xBottom, img_rect.x() - DELTA, img_width + DELTA)
        if not self.axisEnabled(QwtPlot.yLeft):
            self.plot.setAxisScale(
                QwtPlot.yLeft, img_rect.y() - DELTA, img_height + DELTA)

        self.plot.replot()

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
