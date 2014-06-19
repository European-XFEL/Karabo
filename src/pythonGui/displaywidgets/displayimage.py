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


    def valueChanged(self, key, value, timestamp=None):
        if value is None: return

        if self.value is None or value is not self.value:

            # Data type information
            type = value.type.value
            type = hashtypes.Type.fromname[type].numpy

            # Data itself
            data = value.data.value
            npy = np.frombuffer(data, type)

            # Shape
            dimX, dimY = value.dims.value
            npy.shape = dimY, dimX

            # Safety
            if dimX < 1 or dimY < 1: return
                        
            if self.__image is None:
                self.__image = make.image(npy)
                self.__plot.add_item(self.__image)
            else:
                self.__image.set_data(npy)
                self.__plot.replot()
