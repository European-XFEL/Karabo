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

from karabo.common.api import set_initialized_flag
from karabo.common.project.api import ProjectModel
from karabogui.project.restore import ProjectRestoreSession
from karabogui.testing import assert_trait_change


def test_restore_project():
    """A simple test that the finished flag is fired once the project is
    initialized"""
    project = ProjectModel(simple_name="testproject")
    restore = ProjectRestoreSession(project=project)
    with assert_trait_change(restore, "finished"):
        # Reload the project
        set_initialized_flag(project, True)
