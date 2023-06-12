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
from traits.api import Callable, List

from karabogui import icons
from karabogui.graph.common.api import (
    AuxPlots, BaseToolsetController, create_button)


def aux_plots_factory(tool):
    button = None
    if tool is AuxPlots.ProfilePlot:
        button = create_button(icon=icons.beamProfile,
                               checkable=True,
                               tooltip="Beam Profile")
    elif tool == AuxPlots.Histogram:
        button = create_button(icon=icons.histogram,
                               checkable=True,
                               tooltip="Histogram")
    return button


class AuxPlotsToolset(BaseToolsetController):
    tools = List([AuxPlots.ProfilePlot, AuxPlots.Histogram])
    factory = Callable(aux_plots_factory)

    default_tool = AuxPlots.NoPlot

    def select(self, tool):
        """The toolset has can have one or more buttons, with check states
           being exclusive. When a button is unchecked, the toolset returns
           ROITool.NoROI. When selected, on the other hand, the other button
           (if existing), should be unchecked."""

        # Toolset has two buttons, check state are then exclusive
        prof_button = self.buttons[AuxPlots.ProfilePlot]
        hist_button = self.buttons[AuxPlots.Histogram]

        # Uncheck the other button
        if not prof_button.isChecked() and not hist_button.isChecked():
            tool = AuxPlots.NoPlot
        elif tool == AuxPlots.ProfilePlot and prof_button.isChecked():
            hist_button.setChecked(False)
        elif tool == AuxPlots.Histogram and hist_button.isChecked():
            prof_button.setChecked(False)

        super().select(tool)

    def check(self, tool):
        if tool == AuxPlots.NoPlot:
            self.uncheck_all()
            return

        super().check(tool)
