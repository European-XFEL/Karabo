#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 4, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np
from guiqwt.plot import ImageDialog, ImageWidget
from PyQt4.QtCore import Qt, pyqtSignal
from PyQt4.Qwt5.Qwt import QwtPlot

from karabo.middlelayer import EncodingType
from karabogui.dialogs.dialogs import LutRangeDialog


# Ensure correct mapping of reference type to numpy dtype.
REFERENCE_TYPENUM_TO_DTYPE = {
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


# Maps image physical dimension to ndarray dimension
DIMENSIONS = {
    'X': 1,
    'Y': 0,
    'Z': 2
}


def get_image_data(image_node, dimX, dimY, dimZ):
    """ Calculate a numpy array from the given ``image_node`` depending on the
    given dimensions and return it to use for image display.
    In case no data is included, a ``NoneType`` is returned.
    """
    pixels = image_node.pixels.value
    if pixels.data.value is None:
        return None
    if len(pixels.data.value) == 0:
        return None

    arr_type = REFERENCE_TYPENUM_TO_DTYPE[pixels.type.value]
    npy = np.frombuffer(pixels.data.value, dtype=arr_type)
    if dimZ:
        try:
            npy.shape = dimY, dimX, dimZ
        except ValueError:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)
    else:
        try:
            npy.shape = dimY, dimX
        except ValueError:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)

    return npy


def get_dimensions_and_encoding(image_node):
    """ The dimensions and the encoding of the given ``image_node`` are
    returned."""
    dimX = None
    dimY = None
    dimZ = None
    encoding = image_node.encoding.value

    if len(image_node.dims.value) == 2:
        # Shape
        dimX = image_node.dims.value[DIMENSIONS['X']]
        dimY = image_node.dims.value[DIMENSIONS['Y']]
        if encoding == EncodingType.UNDEFINED:
            encoding = EncodingType.GRAY  # assume it's gray
    elif len(image_node.dims.value) == 3:
        # Shape
        dimX = image_node.dims.value[DIMENSIONS['X']]
        dimY = image_node.dims.value[DIMENSIONS['Y']]
        dimZ = image_node.dims.value[DIMENSIONS['Z']]
        if encoding == EncodingType.UNDEFINED:
            if dimZ == 3:
                encoding = EncodingType.RGB  # assume it's RGB
            elif dimZ == 4:
                encoding = EncodingType.RGBA  # assume it's RGBA
            else:
                encoding = EncodingType.GRAY  # assume it's a stack of GRAY

    return dimX, dimY, dimZ, encoding


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
        ``self.get_plot`` to fix basically a guiqwt bug to set the LUT range
        for the color tool bar
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
