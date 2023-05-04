# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

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
