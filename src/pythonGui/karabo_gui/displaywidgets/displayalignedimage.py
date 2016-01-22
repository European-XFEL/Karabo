#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__all__ = ["DisplayAlignedImage"]


from schema import ImageNode
from karabo_gui.widget import DisplayWidget

import numpy as np
from guiqwt.plot import ImageDialog
from guiqwt.builder import make
from karabo.api_2 import Type

from PyQt4.QtGui import (QSlider, QWidget, QHBoxLayout, QVBoxLayout,
                            QComboBox, QSpinBox, QLabel)
from PyQt4.QtCore import Qt
import re

class DisplayAlignedImage(DisplayWidget):
    category = ImageNode
    alias = "Aligned Image View"

    def __init__(self, box, parent):
        super(DisplayAlignedImage, self).__init__(None)
        
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
        self.npys = {}
        self.offsets = {}
        self.rotations = {}
        self.images = {}
        self.pixelRegions = {}
        self.addBox(box)
        self.mins = {}
        self.maxs = {}


    @property
    def boxes(self):
        return list(self.images.keys())


    value = None


    def addBox(self, box):
        #image = make.trimage()
        self.images[box] = None
        #self.plot.add_item(image)
        return True

        

    def cellChanged(self, value):
        self.selectedCell = int(value)
        self.slider.setSliderPosition(int(value))

   
    def axisChanged(self, value):
        self.axis = int(value)
        self.setSlider((next (iter (self.npys.values()))).shape[self.axis])#2-((self.axis+1) % 3)])
        self.sliderMoved(0)
     
    def sliderMoved(self, value):
        self.selectedCell = value
        self.currentCell.setValue(value)
        npy = None
        for box in self.npys:
            if self.axis == 1:
                npy = self.npys[box][:,self.selectedCell,:]
            if self.axis == 0:
                npy = self.npys[box][self.selectedCell,:,:]
            if self.axis == 2:
                npy = self.npys[box][:,:,self.selectedCell]

#            if box not in self.images or len(self.images[box]) == 0:
#                # Some dtypes (eg uint32) are not displayed -> astype('float')
#                self.images[box] = []
#                for i in range(len(self.offsets[box])):
#                    pixelRegion = self.pixelRegions[box][i]
#                    self.images[box] = make.trimage(npy.astype('float'))
#                    self.images[box].set_transform(self.offsets[box][0], self.offsets[box][1], self.rotations[box][0])
#                    self.plot.add_item(self.images[box])
            #else:
            for i in range(len(self.images[box])):
                pixelRegion = self.pixelRegions[box][i]
                if pixelRegion is None:
                    self.images[box][i].set_data(npy.astype('float'))
                else:
                    self.images[box][i].set_data(npy[pixelRegion[1]:pixelRegion[3],
                                             pixelRegion[0]:pixelRegion[2]]
                                             .astype('float'))
            self.updateLutRange()
            self.plot.replot()
                # Also update colormap axis for Image
                #self.plot.update_colormap_axis(self.plot.items[1])
    
    def setSlider(self, dimZ):
        self.slider.setMaximum(dimZ)
        self.currentCell.setMaximum(dimZ)
        if not self.cellWidget.isVisible():
            self.cellWidget.setVisible(True)
        
    def unsetSlider(self):
        self.cellWidget.setVisible(False)
        
    def getAlignmentInformation(self, geometry, leafNodes, tO = [0.,0.,0.], tR = [0.,0.,0.], pixelRegion = None):
        
        
        if hasattr(geometry, "pixelRegion"):
            if pixelRegion == None:
                pixelRegion = geometry.pixelRegion
            else:
                self.log.WARN("Only one pixel region may be defined per leaf")
        
        if hasattr(geometry, "alignment"):
                aO = geometry.alignment.offsets
                aR = geometry.alignment.rotations
                tO = [tO[i] + aO[i] for i in range(3)]
                #tO[0]  = aO[0]*np.cos(tR[0])+aO[0]+tO[0]
               # tO[1]  = aO[0]*np.tan(tR[0])+aO[1]+tO[1]
                #tO[2] += aO[2]
                
                tR = [tR[i] + aR[i] for i in range(3)]
        pattern = re.compile("^(t[0-9]+)")
        hasSub = False
        #schema case
        for att in dir(geometry):
            
            if pattern.match(att):
                   hasSub = True
                   self.getAlignmentInformation(getattr(geometry, att), leafNodes, tO, tR, pixelRegion)
                   #tO = [tO[i] + nO[i] for i in range(3)]
                   #tR = [tR[i] + nR[i] for i in range(3)]

       
        if not hasSub:
            leafNodes.append({"offsets": tO, "rotations": tR, "pixelRegion": pixelRegion})

    def updateLutRange(self):
        minimum = np.min(np.array([k for k in iter(self.mins.values())]))
        maximum = np.max(np.array([k for k in iter(self.maxs.values())]))
        for box in iter(self.images.values()):
            if box is not None:
                for p in box:
                    p.set_lut_range([minimum, maximum])
   

    def valueChanged(self, box, value, timestamp=None):

        if value is None or value.dataType == '0':
            return

        if self.value is not None or value is self.value:
            return

        format = None

        if hasattr(value, "stackAxis"):
            self.axis = value.stackAxis
        
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
            dimX = value.dims[2]
            dimY = value.dims[1]
            dimZ = value.dims[0]                

            if dimZ == 3:
                # Format: RGB
                format = 'QImage.Format_RGB888'

            else:
                
                if self.axis == 0:
                    self.setSlider(dimZ)
                if self.axis == 1:
                    self.setSlider(dimY)
                if self.axis == 2:
                    self.setSlider(dimX)
                
        else:
            return

        # Data type information
        type = value.dataType
        try:
            type = Type.fromname[type].numpy
        except KeyError as e:
            e.message = 'Image element has improper type "{}"'.format(type)
            raise

        # Data itself
        data = value.data
        npy = np.frombuffer(data, type)
        self.npys[box] = npy
        if format == 'QImage.Format_Indexed8':
            try:
                npy.shape = dimY, dimX
                self.npys[box].shape = dimY, dimX
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise

        elif format == 'QImage.Format_RGB888':
            try:
                npy.shape = dimY, dimX, dimZ
                self.npys[box].shape = dimY, dimX, dimZ
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise
        
        else:
            try:
                npy.shape = dimZ, dimY, dimX
                self.npys[box].shape = dimZ, dimY, dimX
                if self.axis ==1:
                    npy = self.npys[box][:,self.selectedCell,:]
                elif self.axis == 0:
                    npy = self.npys[box][self.selectedCell,:,:]
                elif self.axis == 2:
                    npy = self.npys[box][:,:,self.selectedCell]
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                   TrImageItem {}'.format(dimX, dimY, dimZ, len(npy))
                raise

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        if box not in self.images or self.images[box] == None or value.geometry.update:
        #if self.image is None:
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            if box in self.images and self.images[box] is not None:
                self.plot.del_items(self.images[box])

            self.plot.replot()
            self.offsets[box] = []
            self.rotations[box] = []
            self.images[box] = []
            self.pixelRegions[box] = []
            self.mins[box] = np.min(npy)
            self.maxs[box] = np.max(npy)

            

            leafNodes = []
            self.getAlignmentInformation(value.geometry, leafNodes)
            for i, leaf in enumerate(leafNodes):
                offsets = leaf["offsets"]
                rotations = leaf["rotations"]
                pixelRegion = leaf["pixelRegion"]
                image = None
                if pixelRegion is None:
                    image = make.trimage(npy.astype('float'))
                else:
                    image = make.trimage(npy[pixelRegion[1]:pixelRegion[3],
                                             pixelRegion[0]:pixelRegion[2]]
                                                .astype('float'))
                image.set_transform(offsets[0]+(pixelRegion[2]-pixelRegion[0])/2, offsets[1]+(pixelRegion[3]-pixelRegion[1])/2, rotations[0])
                self.offsets[box].append(offsets)
                self.rotations[box].append(rotations)
                self.pixelRegions[box].append(pixelRegion)
                self.images[box].append(image)
                self.plot.add_item_with_z_offset(self.images[box][i], offsets[2])
        else:
                    
            
            self.mins[box] = min(self.mins[box], np.min(npy))
            self.maxs[box] = max(self.maxs[box], np.max(npy))
            for i in range(len(self.images[box])):
                pixelRegion = self.pixelRegions[box][i]
                if pixelRegion is None:
                    self.images[box][i].set_data(npy.astype('float'))
                else:
                    self.images[box][i].set_data(npy[pixelRegion[1]:pixelRegion[3],
                                             pixelRegion[0]:pixelRegion[2]]
                                             .astype('float'))
        
        #self.plot.update_colormap_axis(self.plot.items[0])
            #self.images[box].set_transform(self.offsets[box][0], self.offsets[box][1], self.rotations[box][0])
        self.updateLutRange()
        self.plot.replot()
        
            # Also update colormap axis for Image
            #self.plot.update_colormap_axis(self.plot.items[1])