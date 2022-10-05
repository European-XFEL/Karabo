from unittest import mock

from numpy.testing import assert_array_equal
from pyqtgraph import ColorMap
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.common.scenemodel.api import (
    CrossROIData, ImageGraphModel, RectROIData)
from karabo.native import EncodingType
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.api import COLORMAPS
from karabogui.graph.common.enums import AuxPlots, MouseMode, ROITool
from karabogui.testing import GuiTestCase

from ..image_graph import DisplayImageGraph


class TestImageGraph(GuiTestCase):

    def setUp(self):
        super(TestImageGraph, self).setUp()

        schema = get_pipeline_schema(stack_axis=False)
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)

        self.output_proxy = PropertyProxy(root_proxy=root_proxy,
                                          path='output.data')
        self.img_proxy = PropertyProxy(root_proxy=root_proxy,
                                       path='output.data.image')
        self.controller = DisplayImageGraph(proxy=self.img_proxy)

        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            self.controller.create(None)

    def tearDown(self):
        super(TestImageGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def _click_toolbar(self, tool):
        toolbar = self.controller.widget.toolbar
        toolbar.toolsets[tool.__class__].buttons[tool].click()
        # give some process event time
        self.process_qt_events()

    def test_toolbar_apply(self):
        """The the toolbar apply in the image graph"""
        image_hash = get_image_hash()
        apply_configuration(image_hash, self.output_proxy.binding)

        # Assert current state
        model = self.controller.model
        self.assertEqual(model.aux_plots, AuxPlots.NoPlot)
        self.assertEqual(len(model.roi_items), 0)

        # Toolbar configuration
        self._click_toolbar(AuxPlots.ProfilePlot)
        self._click_toolbar(MouseMode.Picker)

        # Add some ROIs
        roi_controller = self.controller.widget.roi
        roi_controller.add(ROITool.Crosshair, (50, 50), name='Cross 1')
        roi_controller.add(ROITool.Crosshair, (35, 35), name='Cross 2')
        roi_controller.add(ROITool.Rect, (15, 15), (10, 10), name='Rect')

        expected_configs = {
            ROITool.Crosshair: [('Cross 1', (50, 50)), ('Cross 2', (35, 35))],
            ROITool.Rect: [('Rect', (15, 15, 10, 10))]}

        # Assert the expected configuration
        for tool, roi_items in roi_controller._rois.items():
            configs = expected_configs[tool]
            for (name, coords), roi_item in zip(configs, roi_items):
                self.assertEqual(roi_item.name, name)
                self.assertEqual(roi_item.coords, coords)

        apply_action = self.controller.widget.actions()[1]

        # Save to model
        apply_action.triggered.emit(True)
        # give some process event time
        self.process_qt_events()

        model = self.controller.model
        self.assertEqual(model.aux_plots, AuxPlots.ProfilePlot)
        self.assertEqual(len(model.roi_items), 3)

    def test_restore_widget(self):
        """Create a model with ROI settings and restore in Image Graph"""
        model = ImageGraphModel()
        model.aux_plots = AuxPlots.ProfilePlot

        roi_items = [CrossROIData(**{'roi_type': ROITool.Crosshair,
                                     'x': 15, 'y': 15, 'name': 'CrossROI'}),
                     RectROIData(**{'roi_type': ROITool.Rect,
                                    'x': 25, 'y': 25,
                                    'w': 10, 'h': 10,
                                    'name': 'RectROI'})]
        model.roi_items = roi_items

        controller = DisplayImageGraph(proxy=self.img_proxy, model=model)
        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            controller.create(None)

        # Assert the information was loaded
        widget = controller.widget

        # Assert ROI configuration
        expected_roi_config = {ROITool.Crosshair: ('CrossROI', (15, 15)),
                               ROITool.Rect: ('RectROI', (25, 25, 10, 10))}

        for tool, roi_items in widget.roi.roi_items.items():
            roi_item = roi_items[0]
            name, coords = expected_roi_config[tool]
            self.assertEqual(roi_item.coords, coords)
            self.assertEqual(roi_item.name, name)

        menu = widget.plotItem.vb.menu
        self.assertEqual(len(menu.actions()), 3)
        self.assertEqual(menu.actions()[0].text(), "View all")
        self.assertEqual(menu.actions()[1].text(), "")
        self.assertEqual(menu.actions()[2].text(), "Undock")

        path = "karabogui.controllers.display.image_graph.broadcast_event"
        with mock.patch(path) as broadcast:
            menu.actions()[2].trigger()
            broadcast.assert_called_once()
        controller.destroy()

    def test_value_update(self):
        # Restore aux plots
        widget = self.controller.widget
        widget.restore({"aux_plots": AuxPlots.ProfilePlot,
                        "colormap": "viridis"})
        self.assertEqual(widget._aux_plots.current_plot, AuxPlots.ProfilePlot)

        # Test update with dim=3, encoding=GRAY
        image_hash = get_image_hash(dimZ=3, stack_axis=False)
        apply_configuration(image_hash, self.output_proxy.binding)
        self._assert_gray_features(enabled=True)

        # Test update with dim=3, encoding=RGB
        image_hash = get_image_hash(dimZ=3, encoding=EncodingType.RGB,
                                    stack_axis=False)
        apply_configuration(image_hash, self.output_proxy.binding)
        self._assert_gray_features(enabled=False)

        # Now, we update with dim=3, encoding=GRAY again
        image_hash = get_image_hash(dimZ=3, stack_axis=False)
        apply_configuration(image_hash, self.output_proxy.binding)
        self._assert_gray_features(enabled=True)

    def test_undock_image(self):
        """Create a model with ROI settings and undock the Image Graph"""
        model = ImageGraphModel()
        model.aux_plots = AuxPlots.ProfilePlot
        model.undock = True

        roi_items = [CrossROIData(**{'roi_type': ROITool.Crosshair,
                                     'x': 15, 'y': 15,
                                     'name': 'CrossROI'}),
                     RectROIData(**{'roi_type': ROITool.Rect,
                                    'x': 25, 'y': 25,
                                    'w': 10, 'h': 10,
                                    'name': 'RectROI'})]
        model.roi_items = roi_items

        controller = DisplayImageGraph(proxy=self.img_proxy, model=model)
        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            controller.create(None)

        # Assert the information was loaded
        widget = controller.widget

        # Assert ROI configuration
        expected_roi_config = {ROITool.Crosshair: ('CrossROI', (15, 15)),
                               ROITool.Rect: ('RectROI', (25, 25, 10, 10))}

        for tool, roi_items in widget.roi.roi_items.items():
            roi_item = roi_items[0]
            name, coords = expected_roi_config[tool]
            self.assertEqual(roi_item.coords, coords)
            self.assertEqual(roi_item.name, name)
            self.assertFalse(roi_item.translatable)

        menu = controller.widget.plotItem.vb.menu
        self.assertEqual(len(menu.actions()), 1)
        self.assertEqual(menu.actions()[0].text(), "View all")
        controller.destroy()

    def _assert_gray_features(self, enabled):
        widget = self.controller.widget
        cmap = widget.configuration.get("colormap", 'none')
        lut = self._calc_cmap_lut(cmap)

        # Check image item
        ndim = 2 if enabled else 3
        image_item = widget.plotItem.imageItem
        self.assertEqual(image_item.image.ndim, ndim)
        assert_array_equal(image_item.lut, lut)

        # Check colorbar
        if enabled:
            self.assertIsNotNone(widget._colorbar)
            assert_array_equal(widget._colorbar.barItem.lut, lut)

        else:
            self.assertIsNone(widget._colorbar)

        # Check aux plots
        expected_plot = widget.configuration.get("aux_plots", AuxPlots.NoPlot)
        if not enabled:
            expected_plot = AuxPlots.NoPlot
        self.assertEqual(widget._aux_plots.current_plot, expected_plot)

        # Check tool buttons
        buttons = widget.toolbar.toolsets.get(AuxPlots).buttons
        for tool, button in buttons.items():
            self.assertEqual(button.isEnabled(), enabled)
            self.assertEqual(button.isChecked(), tool == expected_plot)

    def _calc_cmap_lut(self, cmap):
        return ColorMap(*zip(*COLORMAPS[cmap])).getLookupTable(alpha=False)
