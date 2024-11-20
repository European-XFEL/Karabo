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

from karabo.native import AccessLevel
from karabogui import mainwindow
from karabogui.testing import singletons


def test_mainwindow(gui_app, mocker, subtests):
    """Small test for verifying that at least the imports are right"""

    network = mocker.Mock()
    manager = mocker.Mock()
    mediator = mocker.Mock()
    krb_access = mocker.patch.object(mainwindow, "krb_access")
    krb_access.HIGHEST_ACCESS_LEVEL = AccessLevel.ADMIN
    krb_access.GLOBAL_ACCESS_LEVEL = AccessLevel.EXPERT
    with singletons(network=network, manager=manager, mediator=mediator):
        logger = mocker.patch("karabogui.logger._logger")
        from karabogui.mainwindow import MainWindow
        mw = MainWindow()
        logger.info.assert_called_with("Started Karabo GUI application ...")
        assert mw is not None

        with subtests.test("Temp session button visibility"):

            krb_access.is_authenticated.return_value = True
            data = {"topic": "foo", "hostname": "exfel", "hostport": 44444}
            mw.update_server_connection(data=data)
            assert mw.tbTempSession.isVisible()

            mw.update_server_connection(data=None)
            assert not mw.tbTempSession.isVisible()
            assert not mw.tbTempSession.isChecked()
            assert mw.tbTempSession.toolTip() == "Start a temporary session"

            krb_access.is_authenticated.return_value = False
            mw.update_server_connection(data=data)
            assert not mw.tbTempSession.isVisible()

    with subtests.test("Test project conflict event in main window"):
        from karabo.common.project.api import ProjectModel
        from karabogui.singletons.project_model import ProjectViewItemModel

        proj = ProjectModel()
        assert proj.conflict is False
        qt_model = ProjectViewItemModel(parent=mw)
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

    with subtests.test("Test the menu bar"):
        menu_bar = mw.menuBar()
        main_menus = [act.text() for act in menu_bar.actions()]
        assert len(main_menus) == 5
        assert main_menus == ["&File", "&Links", "&View", "&Tools", "&Help"]

        help_menu = [action.menu() for action in menu_bar.actions() if
                     action.text() == "&Help"][0]
        assert help_menu
        help_actions = help_menu.actions()
        assert len(help_actions) == 5
        expected = [
            "About", "About Qt", "Tips'N'Tricks", "Check for Updates",
            "Keyboard Shortcuts"]
        assert expected == [action.text() for action in help_actions]

    with subtests.test("Test Access Level"):
        krb_access = mocker.patch.object(mainwindow, "krb_access")
        krb_access.HIGHEST_ACCESS_LEVEL = AccessLevel.ADMIN
        krb_access.GLOBAL_ACCESS_LEVEL = AccessLevel.EXPERT
        access_menu = mw.tbAccessLevel.menu()
        access_levels = [act.text() for act in access_menu.actions()]

        # For User-authenticated login, access level up to
        # 'access.HIGHEST_ACCESS_LEVEL' is exposed
        krb_access.is_authenticated.return_value = True
        mw.onUpdateAccessLevel()
        assert access_levels == [
            'Admin', 'Expert', 'Operator', 'User', 'Observer']

        # For Access-level login, access level up to
        # 'access.HIGHEST_ACCESS_LEVEL' is exposed
        krb_access.is_authenticated.return_value = False
        mw.onUpdateAccessLevel()
        access_levels = [act.text() for act in access_menu.actions()]
        assert access_levels == ['Expert', 'Operator', 'User', 'Observer']
