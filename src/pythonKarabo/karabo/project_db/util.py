__author__ = 'steffen.haufs@xfel.eu'

from eulexistdb import db
import os, re
from subprocess import check_call, Popen, PIPE

#the following two functions are adapted from
# http://stackoverflow.com/questions/38056/how-do-you-check-in-linux-with-
# python-if-a-process-is-still-running
def find_process( process_name ):
    """
    Queries ps to find a process of process_name
    :param process_name:
    :return: the output of the ps command
    """
    ps     = Popen('ps -eaf | grep {}'.format(process_name), shell=True,
                   stdout=PIPE)
    output = ps.stdout.read()
    ps.stdout.close()
    ps.wait()
    return output

def check_running( process_name , path = ""):
    """
    Checks if a process identified by process_name is running
    :param process_name:
    :return: True if the process is running, false otherwise
    """
    output = find_process( process_name )

    if re.search('{}*{}'.format(path, process_name), output) is None:
        return False
    else:
        return True

def assure_running():
    karabo_installation = os.getenv('KARABO', None)
    if karabo_installation is None:
        raise EnvironmentError("The $KARABO environment variable needs"
                               " to be set!")

    #we check if the db is already running
    if not check_running("jetty.jar", karabo_installation):
        #we execute the start script for the database
        print("Starting eXistDB. If this is the first start this may take a while"
              " as it will also install the database first!")
        script_path = "{}/karaboRun/bin/startConfigDB".format(karabo_installation)
        check_call([script_path])








