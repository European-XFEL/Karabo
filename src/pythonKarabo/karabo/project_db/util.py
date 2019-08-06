import os
from time import sleep
from subprocess import check_call

from eulexistdb.db import ExistDB
from eulexistdb.exceptions import ExistDBException
from requests.packages.urllib3.exceptions import HTTPError

from .const import TESTDB_ADMIN_PASSWORD
from .dbsettings import DbSettings


class ProjectDBError(Exception):
    pass


def assure_running(db_settings):
    """
    Assures an instance of existDB is running on your **local** host if the
    KARABO_PROJECT_DB environment variable points to localhost or has not been
    set. Otherwise tries to contact the remote database to assure that the data
    base is reachable
    :param db_settings, a DbSettings object
    :return: None
    """
    karabo_install = os.getenv('KARABO')

    if db_settings.server == 'localhost':
        # we execute the start script for the database.
        # If a database is already started, the script will not start a second
        script_path = os.path.join(karabo_install, 'bin',
                                   'karabo-startconfigdb')
        check_call([script_path])
        # wait until the database is actually up
        maxTimeout = 60
        waitBetween = 5
        count = 0
        while True:
            last_ex = None
            try:
                dbhandle = ExistDB(db_settings.server_url)
                if dbhandle.hasCollection('/system'):
                    return dbhandle
            except (TimeoutError, HTTPError, ExistDBException) as last_ex:
                if count > maxTimeout//waitBetween:
                    raise TimeoutError("Starting project database timed"
                                       " out! Last exception: {}"
                                       .format(last_ex))
            sleep(waitBetween)
            count += 1
    else:
        try:
            dbhandle = ExistDB(db_settings.server_url)
            if db_settings.init_db:
                init_db(db_settings, dbhandle)
            if not dbhandle.hasCollection(db_settings.root_collection):
                raise ProjectDBError("An eXistDB instance without karabo "
                                     "collections was found running on {}."
                                     .format(db_settings.server))
            return dbhandle
        except ExistDBException as e:
            raise ProjectDBError("Could not contact the database server"
                                 " at {}: {}".format(db_settings.server, e))


def get_db_credentials(is_test):
    # in production the KARABO_PROJECT_DB_USER and KARABO_PROJECT_DB_PASSWORD
    # should be set to the username and password of the configDB.
    # Alternatively, the username and password will be defaulted to 'karabo'
    default_user = 'admin' if is_test else 'karabo'
    default_pwd = TESTDB_ADMIN_PASSWORD if is_test else 'karabo'
    user = os.getenv('KARABO_PROJECT_DB_USER', default_user)
    password = os.getenv('KARABO_PROJECT_DB_PASSWORD', default_pwd)
    return user, password


def init_db(db_settings, dbhandle):

    def init_collection(coll_name):
        if not dbhandle.hasCollection(coll_name):
            dbhandle.createCollection(coll_name)
            print(f"Created collection {coll_name}")

    # root
    init_collection(db_settings.root_collection)
    # LOCAL domain
    init_collection(f"{db_settings.root_collection}/LOCAL")
    # test root
    init_collection(db_settings.root_collection_test)
