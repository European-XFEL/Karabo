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
from karabo.native import String
from karabo.project_db.util import DbConnectionNodeBase

from .database import ProjectDatabase


class DbConnectionNode(DbConnectionNodeBase):
    dir = String(defaultValue="projectDB",
                 description="directory where files are saved relative to"
                             " $KARABO/var/data")

    def get_db(self, test_mode=False, init_db=False):
        return ProjectDatabase(
            dir=self.dir, test_mode=test_mode, init_db=init_db)
