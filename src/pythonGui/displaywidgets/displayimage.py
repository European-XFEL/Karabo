#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__all__ = ["DisplayImage"]


from schema import ImageNode
from widget import DisplayWidget

import numpy as np
from guiqwt.plot import ImageDialog
from guiqwt.builder import make
from karabo import hashtypes

class DisplayImage(DisplayWidget):
    category = ImageNode
    alias = "Image View"

    def __init__(self, box, parent):
        super(DisplayImage, self).__init__(box)
        
        self.value = None

        self.widget = ImageDialog(edit=False, toolbar=True,
                                  wintitle=".".join(box.path))
        self.image = None
        self.plot = self.widget.get_plot()


    def valueChanged(self, box, value, timestamp=None):
        if value is None or value.type == '0':
            return

        if self.value is not None or value is self.value:
            return

        if len(value.dims) < 2:
            return

        # Data type information
        type = value.type
        try:
            type = hashtypes.Type.fromname[type].numpy
        except KeyError as e:
            e.message = 'Image element has improper type "{}"'.format(type)
            raise

        # Data itself
        data = value.data
        npy = np.frombuffer(data, type)

        # Shape
        dimX = value.dims[0]
        dimY = value.dims[1]
        
        try:
            npy.shape = dimY, dimX
        except ValueError as e:
            e.message = 'Image has improper shape ({}, {}) for size {}'. \
                format(dimX, dimY, len(npy))
            raise

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        if self.image is None:
            self.image = make.image(npy)
            self.plot.add_item(self.image)
        else:
            self.image.set_data(npy)
            self.plot.replot()
