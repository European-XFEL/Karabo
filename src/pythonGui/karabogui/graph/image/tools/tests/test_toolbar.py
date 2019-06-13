from PyQt4.QtCore import pyqtSlot

from karabogui.testing import GuiTestCase
from karabogui.graph.common.api import (
    AuxPlots, ExportTool, ExportToolset, KaraboToolBar, ROITool, ROIToolset)

from karabogui.graph.image.tools.toolbar import AuxPlotsToolset


class TestCase(GuiTestCase):
    def setUp(self):
        super(TestCase, self).setUp()
        self.toolbar = KaraboToolBar()

    def test_add_aux_plots_toolset(self):
        toolset = self.toolbar.add_toolset(AuxPlotsToolset)
        self.assertEqual(len(self.toolbar.toolset), 2)
        self.assertTrue(AuxPlots in self.toolbar.toolset)
        self.assertTrue(isinstance(self.toolbar.toolset[AuxPlots],
                                   AuxPlotsToolset))
        self.assertTrue(toolset is self.toolbar.toolset[AuxPlots])

    def test_add_roi_toolset(self):
        toolset = self.toolbar.add_toolset(ROIToolset)
        self.assertEqual(len(self.toolbar.toolset), 2)
        self.assertTrue(ROITool in self.toolbar.toolset)
        self.assertTrue(isinstance(self.toolbar.toolset[ROITool], ROIToolset))
        self.assertTrue(toolset is self.toolbar.toolset[ROITool])

    def test_add_export_toolset(self):
        toolset = self.toolbar.add_toolset(ExportToolset)
        self.assertEqual(len(self.toolbar.toolset), 2)
        self.assertTrue(ExportTool in self.toolbar.toolset)
        self.assertTrue(isinstance(self.toolbar.toolset[ExportTool],
                                   ExportToolset))
        self.assertTrue(toolset is self.toolbar.toolset[ExportTool])


class TestAuxPlotsToolset(GuiTestCase):
    def setUp(self):
        super(TestAuxPlotsToolset, self).setUp()
        self.toolset = AuxPlotsToolset()

    def test_button_click(self):
        # Set up test
        tool = None

        @pyqtSlot(object)
        def set_tool(value):
            nonlocal tool
            tool = value

        self.toolset.clicked.connect(set_tool)
        profile_plot_button = self.toolset.buttons[AuxPlots.ProfilePlot]

        # Check if profile plot is checked by default
        self.assertFalse(profile_plot_button.isChecked())

        # Click profile plot button for the first time
        profile_plot_button.click()
        self.assertTrue(tool is AuxPlots.ProfilePlot)
        self.assertTrue(profile_plot_button.isChecked())

        # Click profile plot button again
        profile_plot_button.click()
        self.assertTrue(tool is AuxPlots.NoPlot)
        self.assertFalse(profile_plot_button.isChecked())

    def test_check_button(self):
        profile_plot_button = self.toolset.buttons[AuxPlots.ProfilePlot]

        # Check if profile plot is checked by default
        self.assertFalse(profile_plot_button.isChecked())

        # Set the button as checked
        self.toolset.check_button(AuxPlots.ProfilePlot)
        self.assertTrue(profile_plot_button.isChecked())

    def test_uncheck_all(self):
        for button in self.toolset.buttons.values():
            self.assertFalse(button.isChecked())

        # Set one button as checked just to test
        button.setChecked(True)

        # Uncheck all buttons
        self.toolset.uncheck_all()
        for button in self.toolset.buttons.values():
            self.assertFalse(button.isChecked())


class TestROIToolset(GuiTestCase):
    def setUp(self):
        super(TestROIToolset, self).setUp()
        self.toolset = ROIToolset()

    def test_button_click(self):
        # Set up test
        tool = None

        @pyqtSlot(object)
        def set_tool(value):
            nonlocal tool
            tool = value

        self.toolset.clicked.connect(set_tool)

        # Check if ROI buttons are unchecked by default
        for button in self.toolset.buttons.values():
            self.assertFalse(button.isChecked())

        # Click Rect ROI for the first time
        self.toolset.buttons[ROITool.Rect].click()
        self.assertTrue(tool is ROITool.Rect)
        self.assertTrue(self.toolset.buttons[ROITool.Rect].isChecked())
        self.assertFalse(self.toolset.buttons[ROITool.Crosshair].isChecked())

        # Click Crosshair ROI for the first time
        self.toolset.buttons[ROITool.Crosshair].click()
        self.assertTrue(tool is ROITool.Crosshair)
        self.assertTrue(self.toolset.buttons[ROITool.Crosshair].isChecked())
        self.assertFalse(self.toolset.buttons[ROITool.Rect].isChecked())

        # Click Crosshair ROI again
        self.toolset.buttons[ROITool.Crosshair].click()
        self.assertTrue(tool is ROITool.NoROI)
        self.assertFalse(self.toolset.buttons[ROITool.Crosshair].isChecked())
        self.assertFalse(self.toolset.buttons[ROITool.Rect].isChecked())

    def test_check_button(self):
        rect_roi_button = self.toolset.buttons[ROITool.Rect]

        # Check if profile plot is checked by default
        self.assertFalse(rect_roi_button.isChecked())

        # Set the button as checked
        self.toolset.check_button(ROITool.Rect)
        self.assertTrue(rect_roi_button.isChecked())

    def test_uncheck_all(self):
        for button in self.toolset.buttons.values():
            self.assertFalse(button.isChecked())

        # Set one button as checked just to test
        button.setChecked(True)

        # Uncheck all buttons
        self.toolset.uncheck_all()
        for button in self.toolset.buttons.values():
            self.assertFalse(button.isChecked())
