#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import re

from guiqwt.builder import make
import numpy as np
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QComboBox, QHBoxLayout, QImage, QLabel, QSlider,
                         QSpinBox, QVBoxLayout, QWidget)
from traits.api import Dict, Instance, Int, on_trait_change

from karabo.common.scenemodel.api import DisplayAlignedImageModel
from karabogui.binding.api import ImageBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.images import (
    _DIMENSIONS, get_dimensions_and_format, get_image_data, KaraboImageDialog)
from karabogui.controllers.registry import register_binding_controller


@register_binding_controller(ui_name='Aligned Image View',
                             binding_type=ImageBinding)
class DisplayAlignedImage(BaseBindingController):
    model = Instance(DisplayAlignedImageModel)
    _plot = Instance(object)  # some QWT class...
    _cellWidget = Instance(QWidget)
    _currentCell = Instance(QSpinBox)
    _slider = Instance(QSlider)

    _selectedCell = Int
    _axis = Int
    _npys = Dict
    _offsets = Dict
    _rotations = Dict
    _images = Dict
    _pixelRegions = Dict
    _mins = Dict
    _maxs = Dict

    def add_proxy(self, proxy):
        self._images[proxy] = None
        return True

    def create_widget(self, parent):
        widget = QWidget()
        layout = QHBoxLayout(widget)
        imageWidget = KaraboImageDialog(edit=False, toolbar=True,
                                        parent=parent)
        self._plot = imageWidget.get_plot()
        layout.addWidget(imageWidget)

        self._cellWidget = QWidget()
        cellLayout = QVBoxLayout(self._cellWidget)

        axisLabel = QLabel()
        axisLabel.setText("Indexed axis:")
        cellLayout.addWidget(axisLabel)

        currentAxis = QComboBox()
        currentAxis.addItem("0")
        currentAxis.addItem("1")
        currentAxis.addItem("2")
        currentAxis.setToolTip("Sets the axis over which the cell/pulse "
                               "indexing is performed")
        currentAxis.currentIndexChanged.connect(self._axisChanged)
        cellLayout.addWidget(currentAxis)

        cellSelectLabel = QLabel()
        cellSelectLabel.setText("Pulse/cell:")
        cellLayout.addWidget(cellSelectLabel)

        self._currentCell = QSpinBox()
        self._currentCell.valueChanged.connect(self._cellChanged)
        self._currentCell.setMinimum(0)
        self._currentCell.setToolTip("Sets the pulse/train ID to display")
        cellLayout.addWidget(self._currentCell)

        self._slider = QSlider()
        self._slider.setMinimum(0)
        self._slider.setSingleStep(1)
        self._slider.setTickPosition(QSlider.TicksLeft)
        self._slider.setToolTip("pulse/train ID to display")
        self._slider.valueChanged.connect(self._sliderMoved)
        self._cellWidget.setVisible(False)
        cellLayout.addWidget(self._slider)

        layout.addWidget(self._cellWidget)
        self.add_proxy(self.proxy)
        return widget

    # -------------------------------------------------------------------------

    @pyqtSlot()
    def _cellChanged(self, value):
        self._selectedCell = int(value)
        self._slider.setSliderPosition(int(value))

    @pyqtSlot()
    def _axisChanged(self, value):
        self._axis = int(value)
        array = next(iter(self._npys.values()))
        self._setSlider(array.shape[self._axis])
        self._sliderMoved(0)

    @pyqtSlot()
    def _sliderMoved(self, value):
        self._selectedCell = value
        self._currentCell.setValue(value)
        npy = None
        for proxy in self._npys:
            if self._axis == _DIMENSIONS['X']:
                npy = self._npys[proxy][:, self._selectedCell, :]
            if self._axis == _DIMENSIONS['Y']:
                npy = self._npys[proxy][self._selectedCell, :, :]
            if self._axis == _DIMENSIONS['Z']:
                npy = self._npys[proxy][:, :, self._selectedCell]

            for i in range(len(self._images[proxy])):
                pixelRegion = self._pixelRegions[proxy][i]
                if pixelRegion is None:
                    self._images[proxy][i].set_data(npy.astype('float'))
                else:
                    data = npy[pixelRegion[1]:pixelRegion[3],
                               pixelRegion[0]:pixelRegion[2]].astype('float')
                    self._images[proxy][i].set_data(data)
            self._updateLutRange()
            self._plot.replot()

    def _setSlider(self, dimZ):
        self._slider.setMaximum(dimZ)
        self._currentCell.setMaximum(dimZ)
        if not self._cellWidget.isVisible():
            self._cellWidget.setVisible(True)

    def _unsetSlider(self):
        self._cellWidget.setVisible(False)

    def _getAlignmentInformation(self, geometry, leafNodes, tO=None, tR=None,
                                 pixelRegion=None):
        tO = [0., 0., 0.] if tO is None else tO
        tR = [0., 0., 0.] if tR is None else tR

        if "pixelRegion" in geometry:
            if pixelRegion is None:
                pixelRegion = geometry.pixelRegion.value
            else:
                print("WARN: Only one pixel region may be defined per leaf")

        if "alignment" in geometry:
            aO = geometry.alignment.value.offsets.value
            aR = geometry.alignment.value.rotations.value
            tO = [tO[i] + aO[i] for i in range(3)]
            tR = [tR[i] + aR[i] for i in range(3)]
        pattern = re.compile("^(t[0-9]+)")
        hasSub = False

        # schema case
        for att in geometry.alignment.value:
            if pattern.match(att):
                hasSub = True
                binding = getattr(geometry.alignment.value, att)
                self._getAlignmentInformation(binding.value, leafNodes, tO, tR,
                                              pixelRegion)

        if not hasSub:
            leafNodes.append({"offsets": tO,
                              "rotations": tR,
                              "pixelRegion": pixelRegion})

    def _updateLutRange(self):
        minimum = np.min(np.array([k for k in iter(self._mins.values())]))
        maximum = np.max(np.array([k for k in iter(self._maxs.values())]))
        for stack in self._images.values():
            if stack is not None:
                for p in stack:
                    p.set_lut_range([minimum, maximum])

    @on_trait_change('proxies.binding.config_update')
    def _update_value(self, obj, name, value):
        if name != 'config_update':
            return
        if self.widget is None:
            return

        for proxy in self.proxies:
            if proxy.binding is obj:
                break
        else:
            return

        img_node = proxy.value
        if "stackAxis" in img_node:
            self._axis = img_node.stackAxis.value
        else:
            # this might happen for RGB image
            self._axis = _DIMENSIONS['Z']

        dimX, dimY, dimZ, img_format = get_dimensions_and_format(img_node)
        if dimX is not None and dimY is not None and dimZ is None:
            if self._cellWidget.isVisible():
                self._unsetSlider()
        elif dimZ is not None:
            if self._axis == _DIMENSIONS['Y']:
                self._setSlider(dimZ)
            elif self._axis == _DIMENSIONS['X']:
                self._setSlider(dimY)
            elif self._axis == _DIMENSIONS['Z']:
                self._setSlider(dimX)
        else:
            return

        npy = get_image_data(img_node, dimX, dimY, dimZ, img_format)
        if npy is None:
            return
        self._npys[proxy] = npy

        if img_format is not QImage.Format_Indexed8:
            if self._axis == _DIMENSIONS['X']:
                npy = self._npys[proxy][:, self._selectedCell, :]
            elif self._axis == _DIMENSIONS['Y']:
                npy = self._npys[proxy][self._selectedCell, :, :]
            elif self._axis == _DIMENSIONS['Z']:
                npy = self._npys[proxy][:, :, self._selectedCell]

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        if 'update' not in img_node.geometry.value:
            return

        if (proxy not in self._images or self._images[proxy] is None or
                img_node.geometry.value.update.value):
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            if proxy in self._images and self._images[proxy] is not None:
                self._plot.del_items(self._images[proxy])

            self._plot.replot()
            self._offsets[proxy] = []
            self._rotations[proxy] = []
            self._images[proxy] = []
            self._pixelRegions[proxy] = []
            self._mins[proxy] = np.min(npy)
            self._maxs[proxy] = np.max(npy)

            leafNodes = []
            self._getAlignmentInformation(img_node.geometry.value, leafNodes)
            for i, leaf in enumerate(leafNodes):
                offsets = leaf["offsets"]
                rotations = leaf["rotations"]
                pixelRegion = leaf["pixelRegion"]
                image = None
                if pixelRegion is None:
                    image = make.trimage(npy.astype('float'))
                else:
                    data = npy[pixelRegion[1]:pixelRegion[3],
                               pixelRegion[0]:pixelRegion[2]].astype('float')
                    image = make.trimage(data)

                xoff = offsets[0] + (pixelRegion[2] - pixelRegion[0]) / 2
                yoff = offsets[1] + (pixelRegion[3] - pixelRegion[1]) / 2
                image.set_transform(xoff, yoff, rotations[0])
                self._offsets[proxy].append(offsets)
                self._rotations[proxy].append(rotations)
                self._pixelRegions[proxy].append(pixelRegion)
                self._images[proxy].append(image)
                self._plot.add_item_with_z_offset(self._images[proxy][i],
                                                  offsets[2])
        else:
            self._mins[proxy] = min(self._mins[proxy], np.min(npy))
            self._maxs[proxy] = max(self._maxs[proxy], np.max(npy))
            for i in range(len(self._images[proxy])):
                pixelRegion = self._pixelRegions[proxy][i]
                if pixelRegion is None:
                    self._images[proxy][i].set_data(npy.astype('float'))
                else:
                    data = npy[pixelRegion[1]:pixelRegion[3],
                               pixelRegion[0]:pixelRegion[2]].astype('float')
                    self._images[proxy][i].set_data(data)

        self._updateLutRange()
        self._plot.replot()
