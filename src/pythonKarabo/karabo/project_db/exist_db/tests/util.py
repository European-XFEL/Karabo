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
import os
from subprocess import check_call


def stop_local_database():
    """
    Stops a **locally** running instance of eXistDB if this is the one used
    :return:
    """
    test_db_host = os.getenv('KARABO_TEST_PROJECT_DB', None)
    if test_db_host and test_db_host != 'localhost':
        return
    karabo_install = os.getenv('KARABO')
    script_path = os.path.join(karabo_install, 'bin',
                               'karabo-stopprojectdb')
    check_call([script_path])
