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
import uuid

import pytest

from karabo.common.api import KARABO_PROJECT_MANAGER
from karabo.common.project.api import ProjectModel
from karabo.native import Hash
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.configuration import Configuration
from karabogui.singletons.db_connection import ProjectDatabaseConnection
from karabogui.singletons.mediator import Mediator
from karabogui.testing import singletons
from karabogui.util import process_qt_events


def test_connection(gui_app, mocker):
    mediator = Mediator()

    # Configure the default domain
    config = Configuration()
    domain = f"KaraboGUI-{uuid.uuid4()}"
    config["domain"] = domain

    network = mocker.Mock()
    with singletons(network=network, configuration=config,
                    mediator=mediator):
        assert len(mediator._listeners.keys()) == 0
        db_conn = ProjectDatabaseConnection()
        # db connection registers handlers in the mediator
        assert len(mediator._listeners.keys()) == 4
        assert db_conn.default_domain == domain

        # 1. Get the available domains
        db_conn.get_available_domains()
        network.onListProjectDomains.assert_called_once()

        # 2. Get available projects in domain
        db_conn.get_available_project_data(domain, "project")
        network.onProjectListItems.assert_called_with(
            KARABO_PROJECT_MANAGER, domain, 'project')

        # 3. Get projects
        db_conn.get_projects_with_device(domain, "karaboDevice")
        network.onProjectListProjectsWithDevice.assert_called_with(
            KARABO_PROJECT_MANAGER, domain, "karaboDevice"
        )

        # 4. Read data from project db
        assert db_conn.is_reading() is False
        assert db_conn.is_writing() is False
        assert db_conn.is_processing() is False

        project_uuid = str(uuid.uuid4())
        model = ProjectModel(uuid=project_uuid)
        model.initialized = True

        db_conn.retrieve(domain, project_uuid, model)
        assert db_conn.is_writing() is False
        assert db_conn.is_reading() is True
        assert db_conn.is_processing() is True

        # Network did not send yet
        network.onProjectLoadItems.assert_not_called()
        db_conn.flush()
        network.onProjectLoadItems.assert_called_with(
            KARABO_PROJECT_MANAGER,
            [Hash("domain", domain, "uuid", project_uuid,
                  "item_type", "project")]
        )

        data = {"success": True,
                "items": [Hash("domain", domain, "uuid", project_uuid,
                               "xml", "")]}
        broadcast_event(KaraboEvent.ProjectItemsLoaded, data)
        process_qt_events(timeout=10)
        assert db_conn.is_reading() is False
        assert db_conn.is_processing() is False

        # And the `False` success case
        db_conn.retrieve(domain, project_uuid, model)
        data = {"success": False,
                "items": [Hash("domain", domain, "uuid", project_uuid,
                               "xml", "")]}
        broadcast_event(KaraboEvent.ProjectItemsLoaded, data)
        process_qt_events(timeout=10)
        assert db_conn.is_reading() is False
        assert db_conn.is_processing() is False

        # 5. Save data in project db
        db_conn.store(domain, project_uuid, model)
        assert db_conn.is_writing() is True
        assert db_conn.is_reading() is False
        assert db_conn.is_processing() is True
        network.onProjectSaveItems.assert_not_called()

        db_conn.flush()
        network.onProjectSaveItems.assert_called_once()
        data = {"success": "True",
                "items": [Hash("domain", domain, "uuid", project_uuid,
                               "entry", {}, "success", True,
                               "xml", "")]}

        broadcast_event(KaraboEvent.ProjectItemsSaved, data)
        process_qt_events(timeout=10)
        assert db_conn.is_writing() is False
        assert db_conn.is_processing() is False

        # 6 Save a failed item top level success
        db_conn.store(domain, project_uuid, model)
        db_conn.flush()
        data = {"success": False,
                "items": [Hash("domain", domain, "uuid", project_uuid,
                               "entry", {}, "success", False,
                               "xml", "")]}
        broadcast_event(KaraboEvent.ProjectItemsSaved, data)
        process_qt_events(timeout=10)
        assert db_conn.is_processing() is False

        # 7. Save an item, low level success failure
        db_conn.store(domain, project_uuid, model)
        db_conn.flush()
        assert db_conn.is_processing() is True
        data = {"success": True,
                "items": [Hash("domain", domain, "uuid", project_uuid,
                               "entry", {}, "success", False,
                               "xml", "", "reason", "Conflict")]}
        mbox_path = "karabogui.singletons.db_connection.messagebox"
        mb = mocker.patch(mbox_path)
        broadcast_event(KaraboEvent.ProjectItemsSaved, data)
        process_qt_events(timeout=10)
        assert db_conn.is_processing() is False
        mb.show_error.assert_called_with("Conflict")

        # 8. Update project attribute
        db_conn.update_attribute(domain, "project", project_uuid,
                                 "trashed", True)

        network.onProjectUpdateAttribute.assert_called_with(
            KARABO_PROJECT_MANAGER, [Hash("domain", domain,
                                          "item_type", "project",
                                          "uuid", project_uuid,
                                          "attr_name", "trashed",
                                          "attr_value", True)])

        # 9. Topology events, disconnect and come back with device
        network.reset_mock()
        assert db_conn._have_logged_in is True

        data = {"status": False}
        broadcast_event(KaraboEvent.NetworkConnectStatus, data)
        process_qt_events(timeout=10)
        assert db_conn._have_logged_in is False

        data = {"status": True}
        broadcast_event(KaraboEvent.NetworkConnectStatus, data)
        process_qt_events(timeout=10)
        assert db_conn._have_logged_in is False

        data = {"device": KARABO_PROJECT_MANAGER}
        broadcast_event(KaraboEvent.ProjectDBConnect, data)
        process_qt_events(timeout=10)
        network.onProjectBeginSession.assert_called_once()
        assert db_conn._have_logged_in is True

        # 9. Configuration
        db_conn.default_domain = "New"
        assert config["domain"] == "New"

        # 10. Database scene
        scene_uuid = str(uuid.uuid4())
        token = db_conn.get_database_scene("scene", scene_uuid, 1)
        network.onExecuteGeneric.assert_called_with(
            KARABO_PROJECT_MANAGER, 'slotGetScene',
            Hash(
                "name", "scene",
                "uuid", scene_uuid,
                "domain", "New",
                "db_token", "admin"), token=token)


if __name__ == "__main__":
    pytest.main()
