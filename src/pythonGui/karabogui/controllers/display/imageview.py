#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from guiqwt.builder import make
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QCheckBox, QComboBox, QHBoxLayout, QImage, QLabel,
                         QSlider, QSpinBox, QVBoxLayout, QWidget)
from traits.api import Array, Instance, Int, on_trait_change

from karabo.common.scenemodel.api import DisplayImageModel
from karabogui.binding.api import ImageBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.images import (
    _DIMENSIONS, get_dimensions_and_format, get_image_data, KaraboImageDialog)
from karabogui.controllers.registry import register_binding_controller


# XXX: priority = 10
@register_binding_controller(ui_name='Image View', binding_type=ImageBinding)
class DisplayImage(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(DisplayImageModel)

    # Internal traits
    _image = Instance(object)  # qwt object
    _plot = Instance(object)  # qwt object
    _img_array = Array

    _cellWidget = Instance(QWidget)
    _currentCell = Instance(QSpinBox)
    _slider = Instance(QSlider)
    _cbUpdateColorMap = Instance(QCheckBox)

    _selectedCell = Int(0)
    _axis = Int(2)

    def create_widget(self, parent):
        widget = QWidget()
        layout = QHBoxLayout(widget)

        toolWidget = QWidget()
        toolLayout = QVBoxLayout(toolWidget)

        imageWidget = KaraboImageDialog(edit=False, toolbar=True,
                                        parent=parent)
        self._plot = imageWidget.get_plot()
        layout.addWidget(imageWidget)
        layout.addWidget(toolWidget)

        self._cellWidget = QWidget()
        cellLayout = QVBoxLayout(self._cellWidget)

        self._cbUpdateColorMap = QCheckBox("Autom LUT scale")
        self._cbUpdateColorMap.setChecked(False)
        toolLayout.addWidget(self._cbUpdateColorMap)

        axisLabel = QLabel()
        axisLabel.setText("Indexed axis:")
        cellLayout.addWidget(axisLabel)

        currentAxis = QComboBox()
        currentAxis.addItem("2")
        currentAxis.addItem("1")
        currentAxis.addItem("0")
        currentAxis.setToolTip("Sets the axis over which the cell/pulse "
                               "indexing is performed")
        currentAxis.currentIndexChanged.connect(self._axis_changed)
        cellLayout.addWidget(currentAxis)

        cellSelectLabel = QLabel()
        cellSelectLabel.setText("Pulse/cell:")
        cellLayout.addWidget(cellSelectLabel)

        self._currentCell = QSpinBox()
        self._currentCell.valueChanged.connect(self._cell_changed)
        self._currentCell.setMinimum(0)
        self._currentCell.setToolTip("Sets the pulse/train ID to display")
        cellLayout.addWidget(self._currentCell)

        self._slider = QSlider()
        self._slider.setMinimum(0)
        self._slider.setSingleStep(1)
        self._slider.setTickPosition(QSlider.TicksLeft)
        self._slider.setToolTip("pulse/train ID to display")
        self._slider.valueChanged.connect(self._slider_moved)
        self._cellWidget.setVisible(False)
        cellLayout.addWidget(self._slider)

        toolLayout.addWidget(self._cellWidget)
        return widget

    # ---------------------------------------------------------------------

    @pyqtSlot()
    def _axis_changed(self, value):
        self._axis = int(value)
        self._set_slider(self._img_array.shape[2-((self._axis+1) % 3)])
        self._slider_moved(0)

    @pyqtSlot()
    def _cell_changed(self, value):
        self._selectedCell = int(value)
        self._slider.setSliderPosition(int(value))

    @pyqtSlot()
    def _slider_moved(self, value):
        self._selectedCell = value
        self._currentCell.setValue(value)
        npy = None
        if self._axis == _DIMENSIONS['Y']:
            npy = self._img_array[:, self._selectedCell, :]
        if self._axis == _DIMENSIONS['X']:
            npy = self._img_array[self._selectedCell, :, :]
        if self._axis == _DIMENSIONS['Z']:
            npy = self._img_array[:, :, self._selectedCell]

        self._set_image(npy)

    def _set_slider(self, dimZ):
        self._slider.setMaximum(dimZ)
        self._currentCell.setMaximum(dimZ)
        if not self._cellWidget.isVisible():
            self._cellWidget.setVisible(True)

    def _unset_slider(self):
        self._cellWidget.setVisible(False)

    @on_trait_change('proxies.binding.config_update')
    def _update_image(self, obj, name, value):
        if name != 'config_update':
            return
        if self.widget is None:
            return

        img_node = obj.value
        dimX, dimY, dimZ, format = get_dimensions_and_format(img_node)
        if dimX is not None and dimY is not None and dimZ is None:
            if self._cellWidget.isVisible():
                self._unset_slider()
        elif dimZ is not None:
            if dimZ != 3:
                if self._axis == _DIMENSIONS['Y']:
                    self._set_slider(dimX)
                if self._axis == _DIMENSIONS['X']:
                    self._set_slider(dimY)
                if self._axis == _DIMENSIONS['Z']:
                    self._set_slider(dimZ)
        else:
            return

        array = get_image_data(img_node, dimX, dimY, dimZ, format)
        if array is None:
            return

        self._img_array = array
        if format not in (QImage.Format_Indexed8, QImage.Format_RGB888):
            if self._axis == _DIMENSIONS['Y']:
                array = self._img_array[:, self._selectedCell, :]
            elif self._axis == _DIMENSIONS['X']:
                array = self._img_array[self._selectedCell, :, :]
            elif self._axis == _DIMENSIONS['Z']:
                array = self._img_array[:, :, self._selectedCell]

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        self._set_image(array)

    def _set_image(self, array):
        if self._image is None:
            # Some dtypes (eg uint32) are not displayed -> astype('float')
            self._image = make.image(array.astype('float'))
            self._plot.add_item(self._image)
        else:
            if array.ndim == 2:  # Grayscale
                # This is the current range of the Z axis
                rng = self._image.get_lut_range()
                if rng[0] == rng[1] or self._cbUpdateColorMap.isChecked():
                    # Range was not set, or user selected autoscale
                    rng = None
                # Set new data but keep old Z range
                self._image.set_data(array.astype('float'), lut_range=rng)
            else:
                self._image.set_data(array.astype('float'))
            self._plot.replot()
            if self._cbUpdateColorMap.isChecked():
                # Update colormap axis for image
                self._plot.update_colormap_axis(self._plot.items[1])
