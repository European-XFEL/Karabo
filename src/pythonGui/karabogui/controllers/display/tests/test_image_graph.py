from unittest import mock

from PyQt5.QtWidgets import QGraphicsTextItem

from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabo.common.scenemodel.api import (
    ImageGraphModel, CrossROIData, RectROIData)
from karabogui.controllers.display.tests.image import (
    PipelineData, get_image_hash)
from karabogui.graph.common.enums import MouseMode
from karabogui.graph.common.enums import AuxPlots, ROITool
from karabogui.testing import GuiTestCase

from ..display_image_graph import DisplayImageGraph


class TestImageGraph(GuiTestCase):

    def setUp(self):
        super(TestImageGraph, self).setUp()

        schema = PipelineData().getDeviceSchema()
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
        roi_controller.add(ROITool.Crosshair, (50, 50))
        roi_controller.add(ROITool.Crosshair, (35, 35))
        roi_controller.add(ROITool.Rect, (15, 15), (10, 10))

        expected_configs = {ROITool.Crosshair: [(50, 50), (35, 35)],
                            ROITool.Rect: [(15, 15, 10, 10)]}

        # Assert the expected configuration
        for tool, roi_items in roi_controller._rois.items():
            current = [roi.coords for roi in roi_items]
            expected = expected_configs[tool]
            self.assertListEqual(current, expected)

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
                                     'x': 15, 'y': 15}),
                     RectROIData(**{'roi_type': ROITool.Rect,
                                    'x': 25, 'y': 25,
                                    'w': 10, 'h': 10})]
        model.roi_items = roi_items

        controller = DisplayImageGraph(proxy=self.img_proxy, model=model)
        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            controller.create(None)

        # Assert the information was loaded
        widget = controller.widget

        # Assert ROI configuration
        expected_roi_config = {ROITool.Crosshair: (15, 15),
                               ROITool.Rect: (25, 25, 10, 10)}

        for tool, roi_items in widget.roi.roi_items.items():
            expected_config = expected_roi_config[tool]
            self.assertEqual(roi_items[0].coords, expected_config)
        controller.destroy()
