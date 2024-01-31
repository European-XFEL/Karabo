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
from qtpy.QtCore import Slot

from karabogui.graph.common.api import (
    AuxPlots, ExportTool, ExportToolset, ROITool, ROIToolset,
    ToolbarController)
from karabogui.graph.image.tools.toolbar import AuxPlotsToolset
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.toolbar = ToolbarController()

    def test_add_aux_plots_toolset(self):
        self.toolbar.register_toolset(toolset=AuxPlots, klass=AuxPlotsToolset)
        toolset = self.toolbar.add_toolset(AuxPlots)
        assert len(self.toolbar.toolsets) == 2
        assert AuxPlots in self.toolbar.toolsets
        assert isinstance(self.toolbar.toolsets[AuxPlots],
                          AuxPlotsToolset)
        assert toolset is self.toolbar.toolsets[AuxPlots]

    def test_add_roi_toolset(self):
        toolset = self.toolbar.add_toolset(ROITool)
        assert len(self.toolbar.toolsets) == 2
        assert ROITool in self.toolbar.toolsets
        assert isinstance(self.toolbar.toolsets[ROITool], ROIToolset)
        assert toolset is self.toolbar.toolsets[ROITool]

    def test_add_export_toolset(self):
        toolset = self.toolbar.add_toolset(ExportTool)
        assert len(self.toolbar.toolsets) == 2
        assert ExportTool in self.toolbar.toolsets
        assert isinstance(self.toolbar.toolsets[ExportTool],
                          ExportToolset)
        assert toolset is self.toolbar.toolsets[ExportTool]


class TestAuxPlotsToolset(GuiTestCase):
    def setUp(self):
        super().setUp()
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
        assert not profile_plot_button.isChecked()

        # Click profile plot button for the first time
        profile_plot_button.click()
        assert tool is AuxPlots.ProfilePlot
        assert profile_plot_button.isChecked()

        # Click profile plot button again
        profile_plot_button.click()
        assert tool is AuxPlots.NoPlot
        assert not profile_plot_button.isChecked()

    def test_check(self):
        profile_plot_button = self.toolset.buttons[AuxPlots.ProfilePlot]

        # Check if profile plot is checked by default
        assert not profile_plot_button.isChecked()

        # Set the button as checked
        self.toolset.check(AuxPlots.ProfilePlot)
        assert profile_plot_button.isChecked()

    def test_uncheck_all(self):
        for button in self.toolset.buttons.values():
            assert not button.isChecked()

        # Set one button as checked just to test
        button.setChecked(True)

        # Uncheck all buttons
        self.toolset.uncheck_all()
        for button in self.toolset.buttons.values():
            assert not button.isChecked()


class TestROIToolset(GuiTestCase):
    def setUp(self):
        super().setUp()
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
            assert not button.isChecked()

        # Click Rect ROI for the first time
        self.toolset.buttons[ROITool.Rect].click()
        assert tool is ROITool.Rect
        assert self.toolset.buttons[ROITool.Rect].isChecked()
        assert not self.toolset.buttons[ROITool.Crosshair].isChecked()

        # Click Crosshair ROI for the first time
        self.toolset.buttons[ROITool.Crosshair].click()
        assert tool is ROITool.Crosshair
        assert self.toolset.buttons[ROITool.Crosshair].isChecked()
        assert not self.toolset.buttons[ROITool.Rect].isChecked()

        # Click Crosshair ROI again
        self.toolset.buttons[ROITool.Crosshair].click()
        assert tool is ROITool.NoROI
        assert not self.toolset.buttons[ROITool.Crosshair].isChecked()
        assert not self.toolset.buttons[ROITool.Rect].isChecked()

    def test_check_button(self):
        rect_roi_button = self.toolset.buttons[ROITool.Rect]

        # Check if profile plot is checked by default
        assert not rect_roi_button.isChecked()

        # Set the button as checked
        self.toolset.check(ROITool.Rect)
        assert rect_roi_button.isChecked()

    def test_uncheck_all(self):
        for button in self.toolset.buttons.values():
            assert not button.isChecked()

        # Set one button as checked just to test
        button.setChecked(True)

        # Uncheck all buttons
        self.toolset.uncheck_all()
        for button in self.toolset.buttons.values():
            assert not button.isChecked()
