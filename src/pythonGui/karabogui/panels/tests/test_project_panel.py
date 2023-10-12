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
from karabo.common.project.api import ProjectModel
from karabogui.panels.projectpanel import ProjectPanel
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import singletons


def test_project_panel(gui_app, mocker):
    qt_model = ProjectViewItemModel()
    mediator = mocker.Mock()
    with singletons(project_model=qt_model, mediator=mediator):
        panel = ProjectPanel()
        assert qt_model.root_model is None
        assert repr(panel) == "<ProjectPanel project=None>"
        project_model = ProjectModel(simple_name="Test")
        qt_model.root_model = project_model
        assert repr(panel) == "<ProjectPanel project=Test>"
