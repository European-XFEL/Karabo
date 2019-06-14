from os import path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, pyqtSlot
from PyQt4.QtGui import QWidget
from traits.api import Instance, Int

from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, DetectorGraphModel)

from karabogui.graph.common.api import AuxPlots
from karabogui.graph.image.api import (
    KaraboImagePlot, KaraboImageNode, KaraboImageView)
from karabo.native import EncodingType
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (BaseBindingController, DIMENSIONS,
                                       register_binding_controller)
from karabogui.util import SignalBlocker


class FrameSlider(QWidget):
    axisChanged = pyqtSignal(int)
    cellChanged = pyqtSignal(int)

    def __init__(self, parent=None):
        super(FrameSlider, self).__init__(parent)
        ui_file = op.join(op.dirname(__file__), 'frame_slider.ui')
        uic.loadUi(ui_file, self)

        self.ui_axis.currentIndexChanged.connect(self.axisChanged.emit)
        self.ui_slider_cell.valueChanged.connect(self.on_slider_moved)
        self.ui_cell.valueChanged.connect(self.on_value_changed)

    @pyqtSlot(int)
    def on_slider_moved(self, value):
        with SignalBlocker(self.ui_cell):
            self.ui_cell.setValue(value)
        self.cellChanged.emit(value)

    @pyqtSlot(int)
    def on_value_changed(self, value):
        with SignalBlocker(self.ui_slider_cell):
            self.ui_slider_cell.setSliderPosition(value)
        self.cellChanged.emit(value)

    def set_slider_maximum(self, max_value):
        """Set the maximum possible value and reset the current position

        :param int max_value:
        """
        self.ui_slider_cell.setMaximum(max_value)
        self.ui_cell.setMaximum(max_value)

    def set_axis(self, axis):
        """Sets the combo box axis

        :param axis: the axis value
        :type axis: int
        """
        self.ui_axis.setCurrentIndex(axis)

    def set_cell(self, cell):
        self.ui_cell.setValue(cell)

    def reset(self):
        spinbox_blocker = SignalBlocker(self.ui_cell)
        slider_blocker = SignalBlocker(self.ui_slider_cell)

        with spinbox_blocker, slider_blocker:
            self.ui_slider_cell.setSliderPosition(0)
            self.ui_cell.setValue(0)
        self.cellChanged.emit(0)


AXIS_DEFAULT = 2
FRAME_DEFAULT = 0


@register_binding_controller(ui_name='Detector Graph',
                             klassname='DetectorGraph',
                             binding_type=ImageBinding, priority=10,
                             can_show_nothing=False)
class DisplayDetectorGraph(BaseBindingController):
    model = Instance(DetectorGraphModel, args=())

    # internal traits
    _axis = Int(AXIS_DEFAULT)
    _cell = Int(FRAME_DEFAULT)

    _frame_slider = Instance(FrameSlider)
    _image_node = Instance(KaraboImageNode)
    _plot = Instance(KaraboImagePlot)

    def create_widget(self, parent):
        """
        Setups the Detector Graph Image widget. Differently from the Graph
        Widget, this one has an additional frame on the right side which is
        responsible for controlling which pulse/stack is shown.
        """
        # Setup parent widget
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_picker()
        widget.add_roi()
        widget.add_colorbar()
        widget.add_aux(AuxPlots.ProfilePlot)
        # Finalize
        widget.add_toolbar()

        self._frame_slider = FrameSlider(parent=widget)
        self._frame_slider.axisChanged.connect(self._axisChanged)
        self._frame_slider.cellChanged.connect(self._cellChanged)
        widget.add_widget(self._frame_slider, row=0, col=2)

        # Get a reference for our plotting
        self._plot = widget.plot()

        # QActions
        widget.add_axes_labels_dialog()
        widget.add_transforms_dialog()

        # Restore the model information
        widget.restore(build_graph_config(self.model))

        self._frame_slider.set_axis(AXIS_DEFAULT)
        self._frame_slider.set_cell(FRAME_DEFAULT)

        return widget

    # -----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(int)
    def _axisChanged(self, value):
        if self._image_node is None:
            return
        self._set_slider(self._image_node.get_axis_dimension(value))
        self._frame_slider.reset()
        self._axis = value

    @pyqtSlot(int)
    def _cellChanged(self, value):
        if self._image_node is None:
            return
        np_image = self._image_node.get_slice(self._axis, self._cell)
        self._plot.set_image(np_image)
        self._cell = value

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        self._image_node = KaraboImageNode(proxy.value)
        if not self._image_node.is_valid:
            return

        if not self._image_node.dim_z:
            self._unset_slider()
        elif self._image_node.encoding == EncodingType.GRAY:
            if self._axis == DIMENSIONS['Y']:
                self._set_slider(self._image_node.dim_x)
            if self._axis == DIMENSIONS['X']:
                self._set_slider(self._image_node.dim_y)
            if self._axis == DIMENSIONS['Z']:
                self._set_slider(self._image_node.dim_z)

        # Get region to plot
        np_image = self._image_node.get_slice(self._axis, self._cell)
        self._plot.set_image(np_image)

    def _set_slider(self, dim_z):
        self._frame_slider.set_slider_maximum(dim_z - 1)
        self._frame_slider.setVisible(True)

    def _unset_slider(self):
        """Hide the slider when there's no multiple images"""
        self._frame_slider.setVisible(False)
