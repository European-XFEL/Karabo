from functools import partial

from PyQt4.QtCore import pyqtSlot

from karabogui import icons

from karabogui.graph.common.toolbar import BaseToolsetController, create_tool_button
from karabogui.graph.common.enums import AuxPlots


class AuxPlotsToolset(BaseToolsetController):
    tool_type = AuxPlots

    @pyqtSlot(AuxPlots)
    def _select(self, plot_mode):
        # TODO: Support multiple aux plots buttons
        button = self.buttons[AuxPlots.ProfilePlot]
        if not button.isChecked():
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
        return button

    def check_button(self, plot):
        if plot is not AuxPlots.NoPlot:
            self.buttons[plot].setChecked(True)

    def _default_buttons(self):
        return [AuxPlots.ProfilePlot]
