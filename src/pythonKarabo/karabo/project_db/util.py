import os
from time import sleep
from subprocess import check_call

import psutil
from eulexistdb import db
from eulexistdb.exceptions import ExistDBException
from requests.packages.urllib3.exceptions import HTTPError

from .dbsettings import ProbeDbSettings, LocalDbSettings


class ProjectDBError(Exception):
    pass


def check_running():
    """
    Checks if a DB instance is running
    :return: True if the process is running, false otherwise
    """
    for p in psutil.process_iter():
        cmd = p.cmdline()
        if len(cmd) == 0:
            continue
        # check if a web app for eXistDB is running. the full command is
        # java -jar existDB/start jetty
        if 'java' in cmd and '-jar' in cmd and \
           True in [True if 'eXistDB' in c else False for c in cmd]:
            return True

    return False


def assure_running(project_db_server=None, project_db_port=None):
    """
    Assures an instance of existDB is running on your **local** host if the
    KARABO_PROJECT_DB environment variable points to localhost or has not been
    set. Otherwise tries to contact the remote database to assure that the data
    base is reachable
    :param project_db_server, the server to which to connect, if set to None
     environment variables will be used
    :param project_db_port, the port which to connect to, if set to None
     environment variables will be used.
    :return: None
    """
    karabo_install = os.getenv('KARABO')

    if project_db_server is None:
        project_db_server = os.getenv('KARABO_PROJECT_DB', None)
        if project_db_server is None:  # last try localhost
            project_db_server = "localhost"

    if project_db_port is None:
        project_db_port = os.getenv('KARABO_PROJECT_DB_PORT', 8080)

    if project_db_server is None or project_db_server == 'localhost':
        # we check if the db is already running
        if not check_running():
            # we execute the start script for the database
            script_path = os.path.join(karabo_install, 'bin',
                                       'karabo-startconfigdb')
            check_call([script_path])
            # wait until the database is actually up
            maxTimeout = 60
            waitBetween = 5
            count = 0
            tSettings = ProbeDbSettings(project_db_server,
                                        port=project_db_port)
            while True:
                last_ex = None
                try:

                    dbhandle = db.ExistDB(tSettings.server_url)
                    if dbhandle.hasCollection('/system'):
                        break
                except (TimeoutError, HTTPError, ExistDBException) as last_ex:
                    if count > maxTimeout//waitBetween:
                        raise TimeoutError("Starting project database timed"
                                           " out! Last exception: {}"
                                           .format(last_ex))
                sleep(waitBetween)
                count += 1

            # now init db if needed
            init_local_db()
    else:
        try:
            tSettings = ProbeDbSettings(project_db_server,
                                        port=project_db_port)
            dbhandle = db.ExistDB(tSettings.server_url)
            if not dbhandle.hasCollection(tSettings.root_collection):
                raise ProjectDBError("An eXistDB instance with karabo "
                                     "collections was found running on {}."
                                     .format(project_db_server))
        except ExistDBException as e:
            raise ProjectDBError("Could not contact the database server"
                                 " at {}: {}".format(project_db_server, e))


def stop_database():
    """
    Stops a **locally** running instance of eXistDB
    :return:
    """

    karabo_install = os.getenv('KARABO')
    waitBetween = 5
    if check_running():
        script_path = os.path.join(karabo_install, 'bin',
                                   'karabo-stopconfigdb')
        check_call([script_path])

    # wait til we are down
    while check_running():
        sleep(waitBetween)


def init_local_db():
    """
    Initializes the **local** database structures if not already present
    :return: None
    """

    settings = LocalDbSettings()
    dbhandle = db.ExistDB(settings.server_url)
    krbroot = settings.root_collection

    # root
    if not dbhandle.hasCollection(krbroot):
        dbhandle.createCollection(krbroot)
        print("Created root collection at {}".format(krbroot))
    else:
        print("Root collection already exists at {}".format(krbroot))
        return
    # local domain
    if not dbhandle.hasCollection("{}/LOCAL".format(krbroot)):
        dbhandle.createCollection("{}/LOCAL".format(krbroot))
        print("Created LOCAL domain at {}/LOCAL".format(krbroot))
    else:
        print("Local domain already exists at {}/local".format(krbroot))
    # test root
    if not dbhandle.hasCollection(settings.root_collection_test):
        dbhandle.createCollection(settings.root_collection_test)
        print("Created test collection at {}"
              .format(settings.root_collection_test))
    else:
        print("Test root collection already exists at {}"
              .format(settings.root_collection_test))
