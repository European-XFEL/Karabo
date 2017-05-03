#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 4, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np
from guiqwt.plot import ImageDialog, ImageWidget
from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import QImage
from PyQt4.Qwt5.Qwt import QwtPlot

from karabo_gui.dialogs.dialogs import LutRangeDialog


# Ensure correct mapping of reference type to numpy dtype.
_REFERENCE_TYPENUM_TO_DTYPE = {
    0: 'bool',
    2: 'char',
    4: 'int8',
    6: 'uint8',
    8: 'int16',
    10: 'uint16',
    12: 'int32',
    14: 'uint32',
    16: 'int64',
    18: 'uint64',
    20: 'float',
    22: 'float64'
}


def get_image_data(image_node, dimX, dimY, dimZ, format):
    """ Calculate a numpy array from the given ``image_node`` depending on the
    given dimensions and format and return it to use for image display.
    In case no data is included, a ``NoneType`` is returned.
    """
    pixels = image_node.pixels
    if len(pixels.data) == 0:
        return None

    arr_type = _REFERENCE_TYPENUM_TO_DTYPE[pixels.type]
    npy = np.frombuffer(pixels.data, dtype=arr_type)
    if format is QImage.Format_Indexed8:
        try:
            npy.shape = dimY, dimX
        except ValueError as e:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)
    elif format is QImage.Format_RGB888:
        try:
            npy.shape = dimY, dimX, dimZ
        except ValueError as e:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)
    else:
        try:
            npy.shape = dimY, dimX, dimZ
        except ValueError as e:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)
    return npy


def get_dimensions_and_format(image_node):
    """ The dimensions and the format of the given ``image_node`` are
    returned."""
    dimX = None
    dimY = None
    dimZ = None
    format = None
    if len(image_node.dims) == 2:
        # Shape
        dimX = image_node.dims[1]
        dimY = image_node.dims[0]
        # Format: Grayscale
        format = QImage.Format_Indexed8
    elif len(image_node.dims) == 3:
        # Shape
        dimX = image_node.dims[1]
        dimY = image_node.dims[0]
        dimZ = image_node.dims[2]
        if dimZ == 3:
            # Format: RGB
            format = QImage.Format_RGB888
    return dimX, dimY, dimZ, format


class _KaraboImageMixin(object):
    def __init__(self, **kwargs):
        super(_KaraboImageMixin, self).__init__(**kwargs)
        self.plot = self.get_plot()
        # We are using ImageWidget, which internally creates a BaseImageWidget,
        # which internally creates a CurvePlot or ImagePlot, which contains the
        # method edit_axis_parameters, which creates the widget to set the
        # color axis.
        # It would be a nightmare to overwrite three classes, so we just do
        # a little monkey patching here.
        self.plot.edit_axis_parameters = self._edit_axis_parameters

    def _edit_axis_parameters(self, axis_id):
        """This overwrites the method ``edit_axis_parameter`` of the
        ``self.get_plot`` to fix basically a guiqwt bug to set the LUT range for
        the color tool bar
        """
        if axis_id != QwtPlot.yRight:
            # call the original method that we monkey-patched over
            type(self.plot).edit_axis_parameters(self.plot, axis_id)
        else:
            image_items = self.get_image_item()
            if image_items:
                # Manipulate top item
                last_img = image_items[len(image_items)-1]
                lut_range = last_img.get_lut_range()
                lut_range_full = last_img.get_lut_range_full()
                dialog = LutRangeDialog(lut_range=lut_range,
                                        lut_range_full=lut_range_full,
                                        parent=self)
                if dialog.exec() == dialog.Accepted:
                    last_img.set_lut_range(dialog.lut_range)
                    self.plot.update_colormap_axis(last_img)
                    self.plot.replot()

    def get_image_item(self):
        """Return a list of already existing selectable items of the plot
        """
        plot_items = self.plot.get_items()
        return [item for item in plot_items if item.can_select()]


class KaraboImageDialog(_KaraboImageMixin, ImageDialog):
    """ Possible key arguments:
        * wintitle: window title
        * icon: window icon
        * edit: editable state
        * toolbar: show/hide toolbar
        * options: options sent to the :py:class:`guiqwt.image.ImagePlot`
            object (dictionary)
        * parent: parent widget
        * panels (optional): additionnal panels (list, tuple)
    """


class KaraboImageWidget(_KaraboImageMixin, ImageWidget):
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
        super(KaraboImageWidget, self).__init__(**kwargs)

    def mousePressEvent(self, event):
        """ Overwrite method to forward middle button press event which
        resets the image """
        super(KaraboImageWidget, self).mousePressEvent(event)
        if event.button() is Qt.MidButton:
            self.signal_mouse_event.emit()
