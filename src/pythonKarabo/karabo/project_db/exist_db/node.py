# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.native import AccessLevel, Configurable, String, UInt32

from .database import ProjectDatabase
from .util import get_db_credentials


class ExistDbNode(Configurable):
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
