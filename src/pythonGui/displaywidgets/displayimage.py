#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__all__ = ["DisplayImage"]


from widget import DisplayWidget

import numpy as np
from guiqwt.plot import ImageDialog
from guiqwt.builder import make
from karabo import hashtypes

class DisplayImage(DisplayWidget):
    category = "Image"
    alias = "Image View"

    def __init__(self, box, parent):
        super(DisplayImage, self).__init__(box)
        
        self.value = None

        self.widget = ImageDialog(edit=False, toolbar=True,
                                  wintitle=".".join(box.path))
        self.__image = None
        self.__plot = self.widget.get_plot()


    def valueChanged(self, box, value, timestamp=None):
        if value is None: return

        if self.value is None or value is not self.value:

            # Data type information
            type = value.type.value
            try:
                type = hashtypes.Type.fromname[type].numpy
            except KeyError as e:
                e.message = 'image has improper type "{}"'.format(type)
                raise

            # Data itself
            data = value.data.value
            npy = np.frombuffer(data, type)

            # Shape
            dimX, dimY = value.dims.value
            try:
                npy.shape = dimY, dimX
            except ValueError as e:
                e.message = 'image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise

            # Safety
            if dimX < 1 or dimY < 1:
                raise RuntimeError('image has less than two dimensions')

            if self.__image is None:
                self.__image = make.image(npy)
                self.__plot.add_item(self.__image)
            else:
                self.__image.set_data(npy)
                self.__plot.replot()
