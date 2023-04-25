# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Slot

from karabogui.graph.common.api import (
    AuxPlots, ExportTool, ExportToolset, ROITool, ROIToolset,
    ToolbarController)
from karabogui.graph.image.tools.toolbar import AuxPlotsToolset
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):
    def setUp(self):
        super(TestCase, self).setUp()
        self.toolbar = ToolbarController()

    def test_add_aux_plots_toolset(self):
        self.toolbar.register_toolset(toolset=AuxPlots, klass=AuxPlotsToolset)
        toolset = self.toolbar.add_toolset(AuxPlots)
        self.assertEqual(len(self.toolbar.toolsets), 2)
        self.assertTrue(AuxPlots in self.toolbar.toolsets)
        self.assertTrue(isinstance(self.toolbar.toolsets[AuxPlots],
                                   AuxPlotsToolset))
        self.assertTrue(toolset is self.toolbar.toolsets[AuxPlots])

    def test_add_roi_toolset(self):
        toolset = self.toolbar.add_toolset(ROITool)
        self.assertEqual(len(self.toolbar.toolsets), 2)
        self.assertTrue(ROITool in self.toolbar.toolsets)
        self.assertTrue(isinstance(self.toolbar.toolsets[ROITool], ROIToolset))
        self.assertTrue(toolset is self.toolbar.toolsets[ROITool])

    def test_add_export_toolset(self):
        toolset = self.toolbar.add_toolset(ExportTool)
        self.assertEqual(len(self.toolbar.toolsets), 2)
        self.assertTrue(ExportTool in self.toolbar.toolsets)
        self.assertTrue(isinstance(self.toolbar.toolsets[ExportTool],
                                   ExportToolset))
        self.assertTrue(toolset is self.toolbar.toolsets[ExportTool])


class TestAuxPlotsToolset(GuiTestCase):
    def setUp(self):
        super(TestAuxPlotsToolset, self).setUp()
        self.toolset = AuxPlotsToolset()

    def test_button_click(self):
        # Set up test
        tool = None

        @Slot(object)
        def set_tool(value):
            nonlocal tool
            tool = value

        self.toolset.on_trait_change(set_tool, "current_tool")
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

    def test_check(self):
        profile_plot_button = self.toolset.buttons[AuxPlots.ProfilePlot]

        # Check if profile plot is checked by default
        self.assertFalse(profile_plot_button.isChecked())

        # Set the button as checked
        self.toolset.check(AuxPlots.ProfilePlot)
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

        @Slot(object)
        def set_tool(value):
            nonlocal tool
            tool = value

        self.toolset.on_trait_change(set_tool, "current_tool")

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
        self.toolset.check(ROITool.Rect)
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
