# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
