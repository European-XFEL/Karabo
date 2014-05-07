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


class DisplayImage(DisplayWidget):
    category = "Image"
    alias = "Image View"

    def __init__(self, box, parent):
        self.value = None

        self.widget = ImageDialog(edit=False, toolbar=True,
                                  wintitle=".".join(box.path))
        self.__image = None
        self.__plot = self.widget.get_plot()
        super(DisplayImage, self).__init__(box)


    def valueChanged(self, key, value, timestamp=None):
        if value is None: return

        if self.value is None or value is not self.value:
            dimX, dimY = value.dims.value
            data = value.data.value

            if dimX < 1 or dimY < 1 or len(data) != 4 * dimX * dimY:
                return

            npy = np.frombuffer(data, np.uint8)
            npy.shape = dimY, dimX, 4
            if self.__image is None:
                self.__image = make.image(npy)
                self.__plot.add_item(self.__image)
            else:
                self.__image.set_data(npy)
                self.__plot.replot()
