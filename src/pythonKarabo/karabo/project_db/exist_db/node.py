# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.native import AccessLevel, String, UInt32
from karabo.project_db.util import DbConnectionNodeBase

from .database import ProjectDatabase
from .util import get_db_credentials


class DbConnectionNode(DbConnectionNodeBase):
    host = String(
        defaultValue="localhost",
        displayedName="Database host",
        requiredAccessLevel=AccessLevel.EXPERT)

    # NOTE: the default port is set to 8080 although the port used by the
    #       local project run in the docker container is 8181
    port = UInt32(
        displayedName="Port",
        defaultValue=8080,
        requiredAccessLevel=AccessLevel.EXPERT)

    def get_db(self, test_mode=False, init_db=False):
        user, password = get_db_credentials(test_mode)
        return ProjectDatabase(
            user, password, server=self.host.value, port=self.port.value,
            test_mode=test_mode, init_db=init_db)
