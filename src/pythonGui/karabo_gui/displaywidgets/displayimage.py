#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__all__ = ["DisplayImage"]


from karabo_gui.schema import ImageNode
from karabo_gui.widget import DisplayWidget

import numpy as np
from guiqwt.plot import ImageDialog
from guiqwt.builder import make

from PyQt4.QtGui import (QSlider, QWidget, QHBoxLayout, QVBoxLayout,
                         QCheckBox, QComboBox, QSpinBox, QLabel)


class DisplayImage(DisplayWidget):
    category = ImageNode
    priority = 10
    alias = "Image View"

    def __init__(self, box, parent):
        super(DisplayImage, self).__init__(box)
        
        self.value = None

        self.widget = QWidget()
        self.layout = QHBoxLayout(self.widget)
        self.imageWidget = ImageDialog(edit=False, toolbar=True,
                                  wintitle=".".join(box.path))
        self.image = None
        self.plot = self.imageWidget.get_plot()
        self.layout.addWidget(self.imageWidget)

        self.cellWidget = QWidget()
        self.cellLayout = QVBoxLayout(self.cellWidget)

        self.cbUpdateColorMap = QCheckBox("Automatic intensity scale")
        self.cbUpdateColorMap.setChecked(False)
        self.layout.addWidget(self.cbUpdateColorMap)
              
        self.axisLabel = QLabel()
        self.axisLabel.setText("Indexed axis:")
        self.cellLayout.addWidget(self.axisLabel)
        
        self.currentAxis = QComboBox()
        self.currentAxis.addItem("0")
        self.currentAxis.addItem("1")
        self.currentAxis.addItem("2")
        self.currentAxis.setToolTip("Sets the axis over which the cell/pulse indexing is performed")
        self.currentAxis.currentIndexChanged.connect(self.axisChanged)
        self.cellLayout.addWidget(self.currentAxis)

        self.cellSelectLabel = QLabel()
        self.cellSelectLabel.setText("Pulse/cell:")
        self.cellLayout.addWidget(self.cellSelectLabel)

        self.currentCell = QSpinBox()
        self.currentCell.valueChanged.connect(self.cellChanged)
        self.currentCell.setMinimum(0)
        self.currentCell.setToolTip("Sets the pulse/train ID to display")
        self.cellLayout.addWidget(self.currentCell)


        self.slider = QSlider()
        self.slider.setMinimum(0)
        self.slider.setSingleStep(1)
        self.slider.setTickPosition(QSlider.TicksLeft)
        self.slider.setToolTip("pulse/train ID to display")
        self.slider.valueChanged.connect(self.sliderMoved)
        self.cellWidget.setVisible(False)
        self.cellLayout.addWidget(self.slider)

        self.layout.addWidget(self.cellWidget)
        
        self.selectedCell = 0
        self.axis = 0
        self.npy = None
        

    def cellChanged(self, value):
        self.selectedCell = int(value)
        self.slider.setSliderPosition(int(value))

   
    def axisChanged(self, value):
        self.axis = int(value)
        self.setSlider(self.npy.shape[2-((self.axis+1) % 3)])
        self.sliderMoved(0)
     
    def sliderMoved(self, value):
        self.selectedCell = value
        self.currentCell.setValue(value)
        npy = None
        if self.axis == 0:
            npy = self.npy[:,self.selectedCell,:]
        if self.axis == 1:
            npy = self.npy[self.selectedCell,:,:]
        if self.axis == 2:
            npy = self.npy[:,:,self.selectedCell]
	
        self.setImage(npy)
    
    def setSlider(self, dimZ):
        self.slider.setMaximum(dimZ)
        self.currentCell.setMaximum(dimZ)
        if not self.cellWidget.isVisible():
            self.cellWidget.setVisible(True)
        
    def unsetSlider(self):
        self.cellWidget.setVisible(False)

    def valueChanged(self, box, value, timestamp=None):
        print()
        print("DisplayImage", value)
        if self.value is not None or value is self.value:
            return

        format = None
        if len(value.dims) == 2:
            # Shape
            dimX = value.dims[1]
            dimY = value.dims[0]

            # Format: Grayscale
            format = 'QImage.Format_Indexed8'

            if self.cellWidget.isVisible():
                self.unsetSlider()

        elif len(value.dims) == 3:
            # Shape
            dimX = value.dims[1]
            dimY = value.dims[0]
            dimZ = value.dims[2]

            if dimZ == 3:
                # Format: RGB
                format = 'QImage.Format_RGB888'
            else:
                if self.axis == 0:
                    self.setSlider(dimX)
                if self.axis == 1:
                    self.setSlider(dimY)
                if self.axis == 2:
                    self.setSlider(dimZ)
        else:
            return

        # Data itself
        pixels = value.pixels
        print("pixels.data", pixels.data)
        print("pixels.type", pixels.type)
        print("pixels.shape", pixels.shape)
        if not pixels.shape:
            return
        npy = np.frombuffer(pixels.data, pixels.type)
        self.npy = npy
        if format == 'QImage.Format_Indexed8':
            try:
                npy.shape = dimY, dimX
                self.npy.shape = dimY, dimX
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise
        elif format == 'QImage.Format_RGB888':
            try:
                npy.shape = dimY, dimX, dimZ
                self.npy.shape = dimY, dimX, dimZ
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise
        else:
            try:
                npy.shape = dimY, dimX, dimZ
                self.npy.shape = dimY, dimX, dimZ
                if self.axis == 0:
                    npy = self.npy[:,self.selectedCell,:]
                elif self.axis == 1:
                    npy = self.npy[self.selectedCell,:,:]
                elif self.axis == 2:
                    npy = self.npy[:,:,self.selectedCell]
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        self.setImage(npy)

    def setImage(self, npy):
        if self.image is None:
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            self.image = make.image(npy.astype('float'))
            self.plot.add_item(self.image)

        else:
            if npy.ndim == 2:  # Grayscale
                r = self.image.get_lut_range()  # This is the current range of the Z axis
                if r[0] == r[1] or self.cbUpdateColorMap.isChecked():
                    # Range was not set, or user selected autoscale
                    r = None
                self.image.set_data(npy.astype('float'), lut_range=r)  # Set new data but keep old Z range
            else:
                self.image.set_data(npy.astype('float'))
            self.plot.replot()
            if self.cbUpdateColorMap.isChecked():
                # Update colormap axis for image
                self.plot.update_colormap_axis(self.plot.items[1])
