# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
