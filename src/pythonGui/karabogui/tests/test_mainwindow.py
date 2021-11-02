from unittest import main, mock

from qtpy.QtWidgets import QFrame

from karabogui.testing import GuiTestCase, singletons


class TestCase(GuiTestCase):

    def test_mainwindow(self):
        """Small test for verifying that at least the imports are right"""
        from karabogui.mainwindow import MainWindow

        network = mock.Mock()
        manager = mock.Mock()
        mediator = mock.Mock()
        with singletons(network=network, manager=manager, mediator=mediator):
            mw = MainWindow()
            self.assertIsNotNone(mw)

        with self.subTest("Test project conflict event in main window"):
            from karabo.common.project.api import ProjectModel
            from karabogui.singletons.project_model import ProjectViewItemModel

            proj = ProjectModel()
            self.assertEqual(proj.conflict, False)
            qt_model = ProjectViewItemModel(parent=None)
            qt_model.root_model = proj
            with singletons(project_model=qt_model), \
                    mock.patch("karabogui.mainwindow.get_logger") as logger:
                data = {"uuids": [proj.uuid]}
                mw._event_project_updated(data)
                self.assertEqual(proj.conflict, True)
                logger.assert_called()

        with self.subTest("Test the banner notification"):
            self.assertEqual(mw.notification_banner.frameStyle(),
                             QFrame.NoFrame)

            data = {"message": "Accelerator goes down in 5 min",
                    "trash": "doepianango"}
            mw._event_server_notification(data)
            self.assertEqual(mw.notification_banner.frameStyle(), QFrame.Box)
            self.assertEqual(mw.notification_banner.toPlainText(),
                             "Accelerator goes down in 5 min")

            data = {"message": "", "trash": "doepianango"}
            mw._event_server_notification(data)
            self.assertEqual(mw.notification_banner.toPlainText(), "")
            self.assertEqual(mw.notification_banner.frameStyle(),
                             QFrame.NoFrame)


if __name__ == "__main__":
    main()
