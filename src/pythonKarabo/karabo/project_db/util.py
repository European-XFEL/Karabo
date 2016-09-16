__author__ = 'steffen.haufs@xfel.eu'

from eulexistdb import db
import os, re
from subprocess import check_call, Popen, PIPE
from lxml import etree

from .db_settings import test_db_settings, local_db_settings
from time import sleep


#the following two functions are adapted from
# http://stackoverflow.com/questions/38056/how-do-you-check-in-linux-with-
# python-if-a-process-is-still-running
def find_process( process_name ):
    """
    Queries ps to find a process of process_name
    :param process_name:
    :return: the output of the ps command
    """
    ps     = Popen('ps a | grep "{}"'.format(process_name), shell=True,
                   stdout=PIPE)
    output = ps.stdout.read()
    ps.stdout.close()
    ps.wait()
    return str(output)

def check_running( process_name , path = ""):
    """
    Checks if a process identified by process_name is running
    :param process_name:
    :return: True if the process is running, false otherwise
    """
    output = find_process( process_name )
    if re.search('.*java -jar {}.*{}'.format(path, process_name), output) is None:
        return False
    else:
        return True

def assure_running(project_db_server = None, project_db_port = None):
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
    karabo_installation = os.getenv('KARABO', None)
    if karabo_installation is None:
        raise EnvironmentError("The $KARABO environment variable needs"
                               " to be set!")

    if project_db_server is None:
        project_db_server = os.getenv('KARABO_PROJECT_DB', None)

    if project_db_port is None:
        project_db_port = os.getenv('KARABO_PROJECT_DB_PORT', 8080)

    if project_db_server is None or project_db_server == 'localhost':
        #we check if the db is already running
        if not check_running("start.jar jetty", karabo_installation):
            #we execute the start script for the database
            print("Starting eXistDB. If this is the first start this may take "
                  "a while as it will also install the database first!")
            script_path = "{}/karaboRun/bin/startConfigDB"\
                .format(karabo_installation)
            check_call([script_path])
            sleep(30) #this might take a while
    else:
        try:
            dbhandle = db.ExistDB(test_db_settings(project_db_server,
                                                   port = project_db_port)
                                  .server_url)
            if not dbhandle.hasCollection(test_db_settings(project_db_server).
                                                  root_collection):
                print("An eXistDB instance with karabo collections"
                      " was found running on {}.".format(project_db_server))
            else:
                raise EnvironmentError("Could not contact the database server"
                                   " at {}".format(project_db_server))
        except:
            raise EnvironmentError("Could not contact the database server"
                                   " at {}".format(project_db_server))




def stop_database():
    """
    Stops a **locally** running instance of eXistDB
    :return:
    """
    karabo_installation = os.getenv('KARABO', None)
    if karabo_installation is None:
        raise EnvironmentError("The $KARABO environment variable needs"
                               " to be set!")
    if check_running("start.jar jetty", karabo_installation):
        script_path = "{}/karaboRun/bin/stopConfigDB".format(karabo_installation)
        check_call([script_path])
    else:
        print("No database instance is running! Nothing to stop!")

def init_local_db():
    """
    Initializes the **local** database structures if not already present
    Additionally, versioning is enabled for the data base.
    :return: None
    """

    assure_running()
    settings = local_db_settings()
    dbhandle = db.ExistDB(settings.server_url)
    krbroot = settings.root_collection

    #root
    if not dbhandle.hasCollection(krbroot):
        dbhandle.createCollection(krbroot)
        print("Created root collection at {}".format(krbroot))
    else:
        print("Root collection already exists at {}".format(krbroot))
    #local domain
    if not dbhandle.hasCollection("{}/LOCAL".format(krbroot)):
        dbhandle.createCollection("{}/LOCAL".format(krbroot))
        print("Created LOCAL domain at {}/LOCAL".format(krbroot))
    else:
        print("Local domain already exists at {}/local".format(krbroot))
    #test root
    if not dbhandle.hasCollection(settings.root_collection_test):
        dbhandle.createCollection(settings.root_collection_test)
        print("Created test collection at {}"
              .format(settings.root_collection_test))
    else:
        print("Test root collection already exists at {}"
              .format(settings.root_collection_test))

    #now we make sure we enable versioning by placing the config files in
    #in the appropriate locations

    #here we add the xconf file for version. A stub version of it
    #exists in config_stubs as versioning.xconf.xml
    print("Enabling versioning...")

    loc = os.path.join(os.path.dirname(__file__),
                       './config_stubs/versioning.xconf.xml')

    with open(loc, "r") as f:
        vers_conf_stub = f.read()
        #now set this as configuration for the root collections
        for root in [settings.root_collection,
                           settings.root_collection_test]:

                path = '/db/system/config/db{}'.format(root)

                if not dbhandle.hasCollection(path):
                    dbhandle.createCollection(path)
                    dbhandle.load(vers_conf_stub, "{}/collection.xconf".format(path))
                    print("Added versioning conf to {}".format(path))
                else:
                    print("Versioning already enabled for {}".format(path))


    #now set the appropriate filters in conf.xml
    karabo_installation = os.getenv('KARABO', None)
    if karabo_installation is None:
        raise EnvironmentError("The $KARABO environment variable needs"
                               " to be set!")
    loc_conf = "{}/extern/eXistDB/db/conf.xml".format(karabo_installation)
    conf = etree.parse(loc_conf)

    loc_filter = os.path.join(os.path.dirname(__file__),
                              './config_stubs/versioning_filter.xml')
    filter = etree.parse(loc_filter).getroot()


    serializer = conf.getroot().find('serializer')
    serializer.insert(0, filter)
    str_rep = etree.tostring(conf, pretty_print=True,
                             encoding='UTF-8', xml_declaration=True)\
                            .decode("utf-8")

    with open(loc_conf, "w") as f:
        f.write(str_rep)

    #in the end we have to restart the database
    print("Restarting database...")
    stop_database()
    assure_running()











