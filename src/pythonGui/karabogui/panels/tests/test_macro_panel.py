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
from qtpy.QtWidgets import QDialog

from karabo.common.project.macro import MacroModel
from karabogui.panels.macropanel import MacroPanel
from karabogui.singletons.configuration import Configuration
from karabogui.testing import development_system_hash, singletons
from karabogui.topology.api import SystemTopology


def test_save_dialog(gui_app, mocker):
    """Test the default file name on saving macro from panel"""

    # Name without slash
    model = MacroModel(simple_name="macro_foo")
    panel = MacroPanel(model=model)

    # To avoid actually writing the file mocking the 'open' function.
    mock_method = mocker.patch("builtins.open") # noqa

    mock_dialog = mocker.patch(
        'karabogui.panels.macropanel.getSaveFileName', return_value="foo.py")
    panel.on_save()
    assert mock_dialog.call_args.kwargs['selectFile'] == "macro_foo.py"

    # Name with slash
    panel.model.simple_name = "macro/foo"
    panel.on_save()
    assert mock_dialog.call_args.kwargs['selectFile'] == "macro_foo.py"


def test_on_debug_run(gui_app, mocker):
    """
    Test that 'on_debug_run' shows the 'DebugRunDialog' and runs the macro on
    specific server and set the configuration when the dialog is accepted.
    """
    model = MacroModel(simple_name="macro_foo")
    panel = MacroPanel(model=model)

    topology = SystemTopology()
    topology.initialize(development_system_hash())
    config = Configuration()

    mock_run = mocker.patch("karabogui.panels.macropanel.run_macro_debug")
    mock_dialog = mocker.patch("karabogui.panels.macropanel.DebugRunDialog")

    mock_dialog().exec.return_value = QDialog.Rejected
    with singletons(topology=topology, configuration=config):
        panel.on_debug_run()
        assert mock_run.call_count == 0
        mock_dialog().exec.return_value = QDialog.Accepted
        mock_dialog().comboBox.currentText.return_value = "devserver"
        panel.on_debug_run()
        mock_run.assert_called_once_with(
            model, serverId="devserver", parent=panel)
        assert config['macro_development'] == "devserver"


def test_repr(gui_app):
    model = MacroModel(simple_name="macro_foo")
    panel = MacroPanel(model=model)
    assert repr(panel) == "<MacroPanel macro=macro_foo>"
