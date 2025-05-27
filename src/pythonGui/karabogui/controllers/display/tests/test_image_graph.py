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
import pytest
from numpy.testing import assert_array_equal
from pyqtgraph import ColorMap
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.common.scenemodel.api import (
    CrossROIData, ImageGraphModel, RectROIData)
from karabo.native import Encoding
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.api import COLORMAPS
from karabogui.graph.common.enums import AuxPlots, MouseTool, ROITool

from ..image_graph import DisplayImageGraph


@pytest.fixture()
def image_graph_setup(gui_app, mocker):
    schema = get_pipeline_schema(stack_axis=False)
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding)

    output_proxy = PropertyProxy(root_proxy=root_proxy,
                                 path="output.data")
    img_proxy = PropertyProxy(root_proxy=root_proxy,
                              path="output.data.image")
    controller = DisplayImageGraph(proxy=img_proxy)

    mocker.patch.object(QGraphicsTextItem, "setHtml")
    controller.create(None)

    yield controller, output_proxy, img_proxy

    controller.destroy()
    assert controller.widget is None


def _click_toolbar(gui_app, controller, tool):
    toolbar = controller.widget.toolbar
    toolbar.toolsets[tool.__class__].buttons[tool].click()
    # give some process event time
    gui_app.processEvents()


def _assert_gray_features(controller, enabled):
    widget = controller.widget
    cmap = widget.configuration.get("colormap", "none")
    lut = _calc_cmap_lut(cmap)

    image_item = widget.plotItem.imageItem
    ndim = 2 if enabled else 3
    assert image_item.image.ndim == ndim
    assert_array_equal(image_item.lut, lut)

    if enabled:
        assert widget._colorbar is not None
        assert_array_equal(widget._colorbar.barItem.lut, lut)

    else:
        assert widget._colorbar is None

    expected_plot = widget.configuration.get("aux_plots", AuxPlots.NoPlot)
    if not enabled:
        expected_plot = AuxPlots.NoPlot
    assert widget._aux_plots.current_plot == expected_plot

    buttons = widget.toolbar.toolsets.get(AuxPlots).buttons
    for tool, button in buttons.items():
        assert button.isEnabled() == enabled
        assert button.isChecked() == (tool == expected_plot)


def _calc_cmap_lut(cmap):
    return ColorMap(*zip(*COLORMAPS[cmap])).getLookupTable(alpha=False)


def test_toolbar_apply(gui_app, image_graph_setup):
    """The toolbar apply in the image graph"""
    controller, output_proxy, img_proxy = image_graph_setup
    image_hash = get_image_hash()
    apply_configuration(image_hash, output_proxy.binding)

    # Assert current state
    model = controller.model
    assert model.aux_plots == AuxPlots.NoPlot
    assert len(model.roi_items) == 0

    # Toolbar configuration
    _click_toolbar(gui_app, controller, AuxPlots.ProfilePlot)
    _click_toolbar(gui_app, controller, MouseTool.Picker)

    # Add some ROIs
    roi_controller = controller.widget.roi
    roi_controller.add(ROITool.Crosshair, (50, 50), name="Cross 1")
    roi_controller.add(ROITool.Crosshair, (35, 35), name="Cross 2")
    roi_controller.add(ROITool.Rect, (15, 15), (10, 10), name="Rect")

    expected_configs = {
        ROITool.Crosshair: [("Cross 1", (50, 50)), ("Cross 2", (35, 35))],
        ROITool.Rect: [("Rect", (15, 15, 10, 10))]}

    # Assert the expected configuration
    for tool, roi_items in roi_controller._rois.items():
        configs = expected_configs[tool]
        for (name, coords), roi_item in zip(configs, roi_items):
            assert roi_item.name == name
            assert roi_item.coords == coords

    apply_action = controller.widget.actions()[2]
    assert apply_action.text() == "Set ROI and Aux"

    # Save to model
    apply_action.triggered.emit(True)
    # give some process event time
    gui_app.processEvents()

    model = controller.model
    assert model.aux_plots == AuxPlots.ProfilePlot
    assert len(model.roi_items) == 3


def test_restore_widget(image_graph_setup, mocker):
    """Create a model with ROI settings and restore in Image Graph"""
    _, _, img_proxy = image_graph_setup
    model = ImageGraphModel()
    model.aux_plots = AuxPlots.ProfilePlot

    roi_items = [CrossROIData(**{"roi_type": ROITool.Crosshair,
                                 "x": 15, "y": 15, "name": "CrossROI"}),
                 RectROIData(**{"roi_type": ROITool.Rect,
                                "x": 25, "y": 25,
                                "w": 10, "h": 10,
                                "name": "RectROI"})]
    model.roi_items = roi_items

    controller = DisplayImageGraph(proxy=img_proxy, model=model)
    mocker.patch.object(QGraphicsTextItem, "setHtml")
    controller.create(None)

    # Assert the information was loaded
    widget = controller.widget

    # Assert ROI configuration
    expected_roi_config = {ROITool.Crosshair: ("CrossROI", (15, 15)),
                           ROITool.Rect: ("RectROI", (25, 25, 10, 10))}

    for tool, roi_items in widget.roi.roi_items.items():
        roi_item = roi_items[0]
        name, coords = expected_roi_config[tool]
        assert roi_item.coords == coords
        assert roi_item.name == name

    menu = widget.plotItem.vb.menu
    assert len(menu.actions()) == 3
    assert menu.actions()[0].text() == "View all"
    assert menu.actions()[1].text() == ""
    assert menu.actions()[2].text() == "Undock"

    path = "karabogui.controllers.display.image_graph.broadcast_event"
    broadcast = mocker.patch(path)
    menu.actions()[2].trigger()
    broadcast.assert_called_once()


def test_value_update(image_graph_setup):
    # Restore aux plots
    controller, output_proxy, _ = image_graph_setup
    widget = controller.widget
    widget.restore({"aux_plots": AuxPlots.ProfilePlot,
                    "colormap": "viridis"})
    assert widget._aux_plots.current_plot == AuxPlots.ProfilePlot

    # Test update with dim=3, encoding=GRAY
    image_hash = get_image_hash(dimZ=3, stack_axis=False)
    apply_configuration(image_hash, output_proxy.binding)
    _assert_gray_features(controller, enabled=True)

    # Test update with dim=3, encoding=RGB
    image_hash = get_image_hash(dimZ=3, encoding=Encoding.RGB,
                                stack_axis=False)
    apply_configuration(image_hash, output_proxy.binding)
    _assert_gray_features(controller, enabled=False)

    # Now, we update with dim=3, encoding=GRAY again
    image_hash = get_image_hash(dimZ=3, stack_axis=False)
    apply_configuration(image_hash, output_proxy.binding)
    _assert_gray_features(controller, enabled=True)
    assert controller._plot.imageItem.image.shape == (30, 40)

    # Erase widget, a default with size 10, 10 is set
    controller.clear_widget()
    assert controller._plot.imageItem.image.shape == (10, 10)


def test_undock_image(image_graph_setup, mocker):
    """Create a model with ROI settings and undock the Image Graph"""
    _, _, img_proxy = image_graph_setup
    model = ImageGraphModel()
    model.aux_plots = AuxPlots.ProfilePlot
    model.undock = True

    roi_items = [CrossROIData(**{"roi_type": ROITool.Crosshair,
                                 "x": 15, "y": 15, "name": "CrossROI"}),
                 RectROIData(**{"roi_type": ROITool.Rect,
                                "x": 25, "y": 25,
                                "w": 10, "h": 10,
                                "name": "RectROI"})]
    model.roi_items = roi_items

    controller = DisplayImageGraph(proxy=img_proxy, model=model)
    mocker.patch.object(QGraphicsTextItem, "setHtml")
    controller.create(None)

    # Assert the information was loaded
    widget = controller.widget

    # Assert ROI configuration
    expected_roi_config = {ROITool.Crosshair: ("CrossROI", (15, 15)),
                           ROITool.Rect: ("RectROI", (25, 25, 10, 10))}

    for tool, roi_items in widget.roi.roi_items.items():
        roi_item = roi_items[0]
        name, coords = expected_roi_config[tool]
        assert roi_item.coords == coords
        assert roi_item.name == name
        assert not roi_item.translatable

    menu = controller.widget.plotItem.vb.menu
    assert len(menu.actions()) == 1
    assert menu.actions()[0].text() == "View all"


def test_save_color_levels(gui_app, image_graph_setup):
    controller, output_proxy, img_proxy = image_graph_setup
    image_hash = get_image_hash()
    apply_configuration(image_hash, output_proxy.binding)

    # When no levels saved.
    model = controller.model
    assert model.color_levels == []

    # Change the levels value.
    widget = controller.widget
    levels = [10, 45]
    widget.plotItem.set_image_levels(levels)
    assert model.color_levels == []

    save_action = controller.widget.actions()[1]
    assert save_action.text() == "Save Color Levels"
    save_action.triggered.emit(True)
    gui_app.processEvents()

    model = controller.model
    assert model.color_levels == [10, 45]

    # Set auto-levels
    widget.plotItem.set_image_levels(None)
    save_action.triggered.emit(True)
    gui_app.processEvents()

    model = controller.model
    assert model.color_levels == []
