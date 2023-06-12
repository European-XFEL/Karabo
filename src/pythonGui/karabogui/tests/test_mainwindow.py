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
from qtpy.QtWidgets import QFrame

from karabogui.testing import singletons


def test_mainwindow(gui_app, mocker, subtests):
    """Small test for verifying that at least the imports are right"""

    network = mocker.Mock()
    manager = mocker.Mock()
    mediator = mocker.Mock()
    with singletons(network=network, manager=manager, mediator=mediator):
        logger = mocker.patch("karabogui.logger._logger")
        from karabogui.mainwindow import MainWindow
        mw = MainWindow()
        logger.info.assert_called_with("Started Karabo GUI application ...")
        assert mw is not None

    with subtests.test("Test project conflict event in main window"):
        from karabo.common.project.api import ProjectModel
        from karabogui.singletons.project_model import ProjectViewItemModel

        proj = ProjectModel()
        assert proj.conflict is False
        qt_model = ProjectViewItemModel(parent=None)
        qt_model.root_model = proj
        with singletons(project_model=qt_model):
            logger = mocker.patch("karabogui.mainwindow.get_logger")
            data = {"uuids": [proj.uuid]}
            mw._event_project_updated(data)
            assert proj.conflict is True
            logger.assert_called()

    with subtests.test("Test the banner notification"):
        assert mw.notification_banner.frameStyle() == QFrame.NoFrame

        data = {"message": "Accelerator goes down in 5 min",
                "trash": "doepianango"}
        mw._event_server_notification(data)
        assert mw.notification_banner.frameStyle() == QFrame.Box
        t = "Accelerator goes down in 5 min"
        assert mw.notification_banner.toPlainText() == t

        data = {"message": "", "trash": "doepianango"}
        mw._event_server_notification(data)
        assert mw.notification_banner.toPlainText() == ""
        assert mw.notification_banner.frameStyle() == QFrame.NoFrame
