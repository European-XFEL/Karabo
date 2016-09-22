import os
from time import sleep
from subprocess import check_call

import psutil
from lxml import etree
from eulexistdb import db
from eulexistdb.exceptions import ExistDBException
from requests.packages.urllib3.exceptions import (NewConnectionError,
                                                  ConnectTimeoutError,
                                                  ConnectionError,
                                                  HTTPError,
                                                  ResponseError,
                                                  RequestError,
                                                  MaxRetryError,
                                                  ProtocolError)


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
        if 'java' in cmd and '-jar' in cmd and 'jetty' in cmd and \
           'eXistDB/start' in cmd[2]:
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

    if project_db_port is None:
        project_db_port = os.getenv('KARABO_PROJECT_DB_PORT', 8080)

    if project_db_server is None or project_db_server == 'localhost':
        # we check if the db is already running
        if not check_running():
            # we execute the start script for the database
            script_path = os.path.join(karabo_install, 'karaboRun', 'bin',
                                       'startConfigDB')
            check_call([script_path])
            # wait until the database is acutally up
            maxTimeout = 60
            waitBetween = 5
            count = 0
            last_ex = None
            while True:
                if count > maxTimeout//waitBetween:
                    raise TimeoutError("Starting project database timed out!"
                                       "Last exception: {}".format(last_ex))
                try:
                    tSettings = ProbeDbSettings(project_db_server,
                                                port=project_db_port)
                    dbhandle = db.ExistDB(tSettings.server_url)
                    if dbhandle.hasCollection('/system'):
                        break
                except (TimeoutError, HTTPError) as last_ex:
                    sleep(waitBetween)
                count += 1
    else:
        try:
            tSettings = ProbeDbSettings(project_db_server,
                                        port=project_db_port)
            dbhandle = db.ExistDB(tSettings.server_uri)
            if not dbhandle.hasCollection(tSettings.root_collection):
                raise ProjectDBError("An eXistDB instance with karabo "
                                     "collections was found running on {}."
                                     .format(project_db_server))
            else:
                raise ProjectDBError("Could not contact the database server"
                                     " at {}".format(project_db_server))
        except ExistDBException as e:
            raise ProjectDBError("Could not contact the database server"
                                 " at {}: {}".format(project_db_server, e))


def stop_database():
    """
    Stops a **locally** running instance of eXistDB
    :return:
    """
    karabo_install = os.getenv('KARABO')
    if check_running():
        script_path = os.path.join(karabo_install, 'karaboRun', 'bin',
                                   'stopConfigDB')
        check_call([script_path])


def init_local_db():
    """
    Initializes the **local** database structures if not already present
    Additionally, versioning is enabled for the data base.
    :return: None
    """

    assure_running()
    settings = LocalDbSettings()
    dbhandle = db.ExistDB(settings.server_url)
    krbroot = settings.root_collection

    # root
    if not dbhandle.hasCollection(krbroot):
        dbhandle.createCollection(krbroot)
        print("Created root collection at {}".format(krbroot))
    else:
        print("Root collection already exists at {}".format(krbroot))
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

    # now we make sure we enable versioning by placing the config files in
    # in the appropriate locations

    # here we add the xconf file for version. A stub version of it
    # exists in config_stubs as versioning.xconf.xml
    print("Enabling versioning...")

    loc = os.path.join(os.path.dirname(__file__),
                       'config_stubs', 'versioning.xconf.xml')

    with open(loc, "r") as f:
        vers_conf_stub = f.read()
        # now set this as configuration for the root collections
        for root in [settings.root_collection,
                     settings.root_collection_test]:

                path = '/system/config/db{}'.format(root)

                if not dbhandle.hasCollection(path):
                    dbhandle.createCollection(path)
                    dbhandle.load(vers_conf_stub,
                                  "{}/collection.xconf".format(path))
                    print("Added versioning conf to {}".format(path))
                else:
                    print("Versioning already enabled for {}".format(path))

    # now set the appropriate filters in conf.xml
    karabo_install = os.getenv('KARABO', None)
    if karabo_install is None:
        raise EnvironmentError("The $KARABO environment variable needs"
                               " to be set!")
    loc_conf = os.path.join(karabo_install, 'extern', 'eXistDB', 'db',
                            'conf.xml')
    conf = etree.parse(loc_conf)

    loc_filter = os.path.join(os.path.dirname(__file__),
                              'config_stubs', 'versioning_filter.xml')
    filter = etree.parse(loc_filter).getroot()

    serializer = conf.getroot().find('serializer')
    serializer.insert(0, filter)
    b_rep = etree.tostring(conf, pretty_print=True,
                           encoding='UTF-8', xml_declaration=True)
    str_rep = b_rep.decode("utf-8")

    with open(loc_conf, "w") as f:
        f.write(str_rep)

    # in the end we have to restart the database
    stop_database()
    assure_running()
