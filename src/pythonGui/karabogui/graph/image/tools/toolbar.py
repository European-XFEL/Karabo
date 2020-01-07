from functools import partial

from PyQt5.QtCore import pyqtSlot

from karabogui import icons

from karabogui.graph.common.api import (
    AuxPlots, BaseToolsetController, create_tool_button)


class AuxPlotsToolset(BaseToolsetController):
    tool_type = AuxPlots

    @pyqtSlot(AuxPlots)
    def _select(self, plot_mode):
        """The toolset has can have one or more buttons, with check states
           being exclusive. When a button is unchecked, the toolset returns
           ROITool.NoROI. When selected, on the other hand, the other button
           (if existing), should be unchecked."""
        if set(self._default_buttons()) == set(self.buttons.keys()):
            # Toolset has two buttons, check state are then exclusive
            prof_button = self.buttons[AuxPlots.ProfilePlot]
            hist_button = self.buttons[AuxPlots.Histogram]

            # Uncheck the other button
            if not prof_button.isChecked() and not hist_button.isChecked():
                plot_mode = AuxPlots.NoPlot
            elif plot_mode is AuxPlots.ProfilePlot and prof_button.isChecked():
                hist_button.setChecked(False)
            elif plot_mode is AuxPlots.Histogram and hist_button.isChecked():
                prof_button.setChecked(False)
        else:
            # Only one button is present.
            if not self.buttons[plot_mode].isChecked():
                plot_mode = AuxPlots.NoPlot

        super(AuxPlotsToolset, self)._select(plot_mode)

    def _button_factory(self, aux_plots):
        button = None
        if aux_plots is AuxPlots.ProfilePlot:
            button = create_tool_button(
                icon=icons.beamProfile,
                checkable=True,
                tooltip="Beam Profile",
                on_clicked=partial(self._select, AuxPlots.ProfilePlot)
            )
        elif aux_plots is AuxPlots.Histogram:
            button = create_tool_button(
                icon=icons.histogram,
                checkable=True,
                tooltip="Histogram",
                on_clicked=partial(self._select, AuxPlots.Histogram)
            )

        return button

    def check_button(self, plot):
        if plot is not AuxPlots.NoPlot:
            self.buttons[plot].setChecked(True)

    def _default_buttons(self):
        return [AuxPlots.ProfilePlot, AuxPlots.Histogram]
