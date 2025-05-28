# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy import uic
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QWidget
from traits.api import Enum, Instance, Int, WeakRef

from karabo.common.scenemodel.api import (
    DetectorGraphModel, build_graph_config, restore_graph_config)
from karabo.native import Encoding, Timestamp
from karabogui.binding.api import ImageBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import AuxPlots, Axes
from karabogui.graph.image.api import (
    KaraboImageNode, KaraboImagePlot, KaraboImageView, karabo_default_image)
from karabogui.util import SignalBlocker

from .util import get_ui_file


class FrameSlider(QWidget):
    axisChanged = Signal(str)
    cellChanged = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        ui_file = get_ui_file("frame_slider.ui")
        uic.loadUi(ui_file, self)

        # Populate our axis combobox
        self.cb_axis.addItems([Axes.X.name, Axes.Y.name, Axes.Z.name])

        self.cb_axis.currentIndexChanged[str].connect(self.axisChanged.emit)
        self.sl_cell.valueChanged.connect(self.on_slider_moved)
        self.sb_cell.valueChanged.connect(self.on_value_changed)

    def on_slider_moved(self, value):
        with SignalBlocker(self.sb_cell):
            self.sb_cell.setValue(value)
        self.cellChanged.emit(value)

    def on_value_changed(self, value):
        with SignalBlocker(self.sl_cell):
            self.sl_cell.setSliderPosition(value)
        self.cellChanged.emit(value)

    def set_slider_maximum(self, max_value):
        """Set the maximum possible value and reset the current position

        :param int max_value:
        """
        self.sl_cell.setMaximum(max_value)
        self.sb_cell.setMaximum(max_value)

    def set_axis(self, axis):
        """Sets the combo box axis based on the given axis name

        :param axis: the selected axes
        :type axis: Enum (Axes)
        """
        self.cb_axis.setCurrentIndex(Axes(axis).value)

    def set_cell(self, cell):
        self.sb_cell.setValue(cell)

    def reset(self):
        spinbox_blocker = SignalBlocker(self.sb_cell)
        slider_blocker = SignalBlocker(self.sl_cell)

        with spinbox_blocker, slider_blocker:
            self.sl_cell.setSliderPosition(0)
            self.sb_cell.setValue(0)
        self.cellChanged.emit(0)


@register_binding_controller(ui_name="Detector Graph",
                             klassname="DetectorGraph",
                             binding_type=ImageBinding, priority=10,
                             can_show_nothing=False)
class DisplayDetectorGraph(BaseBindingController):
    model = Instance(DetectorGraphModel, args=())

    # internal traits
    _axis = Enum(*Axes)
    _cell = Int(0)

    _frame_slider = Instance(FrameSlider)
    _image_node = Instance(KaraboImageNode, args=())
    _plot = WeakRef(KaraboImagePlot)

    def create_widget(self, parent):
        """
        Setups the Detector Graph Image widget. Differently from the Graph
        Widget, this one has an additional frame on the right side which is
        responsible for controlling which pulse/stack is shown.
        """
        # Setup parent widget
        widget = KaraboImageView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.toolTipChanged.connect(self.show_timestamp_tooltip)
        widget.add_picker()
        widget.add_roi()
        widget.add_colorbar()
        widget.add_aux(AuxPlots.ProfilePlot, smooth=True)
        widget.add_aux(AuxPlots.Histogram)
        # Finalize
        widget.add_toolbar()

        self._frame_slider = FrameSlider(parent=widget)
        self._frame_slider.axisChanged.connect(self._axis_changed)
        self._frame_slider.cellChanged.connect(self._cell_changed)
        widget.add_widget(self._frame_slider, row=0, col=2)
        self._frame_slider.setVisible(False)

        # Get a reference for our plotting
        self._plot = widget.plot()

        # QActions
        widget.add_axes_labels_dialog()
        widget.add_transforms_dialog()

        # Default axes
        self._axis = Axes.Z

        # Restore the model information
        widget.restore(build_graph_config(self.model))

        self._frame_slider.set_axis(self._axis)
        self._frame_slider.set_cell(self._cell)

        return widget

    def clear_widget(self):
        self._plot.setData(karabo_default_image())

    # -----------------------------------------------------------------------
    # Qt Slots

    def show_timestamp_tooltip(self):
        image_node = self.proxy.value
        if image_node is None or getattr(image_node, "pixels", None) is None:
            # No schema for output channel, pixels not available
            return
        timestamp = image_node.pixels.value.data.timestamp
        if timestamp is None:
            # Can happen when we toggle aux plots
            return
        diff = Timestamp().toTimestamp() - timestamp.toTimestamp()
        self.widget.setToolTip("{} --- Last image received {:.3f} s "
                               "ago".format(self.proxy.key, diff))

    def _axis_changed(self, axis):
        """Called whenever an axis is selected in the FrameSlider

        :param axis: the selected axis
        :type axis: basestring
        """
        if self._image_node is None:
            return

        self._axis = Axes[axis]
        self._set_slider_max(self._image_node.get_axis_dimension(self._axis))
        self._frame_slider.reset()

    def _cell_changed(self, value):
        if self._image_node is None:
            return

        self._cell = value
        self._update_image()

    def _update_image(self):
        # Get region to plot
        np_image = self._image_node.get_slice(self._axis, self._cell)
        self._plot.set_image(np_image)

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    # -----------------------------------------------------------------------

    def value_update(self, proxy):
        self._image_node.set_value(proxy.value)

        if not self._image_node.is_valid:
            return

        # Hide the slider when there's no multiple images
        self._frame_slider.setVisible(self._image_node.dim_z != 0)

        if self._image_node.encoding == Encoding.GRAY:
            slider_max = self._image_node.get_axis_dimension(self._axis)
            self._set_slider_max(slider_max)

        self._update_image()

    def _set_slider_max(self, value):
        max_value = max(value - 1, 0)
        self._frame_slider.set_slider_maximum(max_value)
