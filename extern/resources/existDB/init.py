from time import sleep

from eulexistdb import db
from eulexistdb.exceptions import ExistDBException
from requests.packages.urllib3.exceptions import HTTPError

SERVER_URL = "http://admin:karabo@localhost:8080/exist/xmlrpc"
ROOT_COLLECTION = "/krb_config"
ROOT_COLLECTION_TEST = "/krb_test"


def init_local_db(dbhandle):
    # root
    if not dbhandle.hasCollection(ROOT_COLLECTION):
        dbhandle.createCollection(ROOT_COLLECTION)
        print("Created root collection at {}".format(ROOT_COLLECTION))

    # local domain
    if not dbhandle.hasCollection("{}/LOCAL".format(ROOT_COLLECTION)):
        dbhandle.createCollection("{}/LOCAL".format(ROOT_COLLECTION))
        print("Created LOCAL domain at {}/LOCAL".format(ROOT_COLLECTION))

    # test root
    if not dbhandle.hasCollection(ROOT_COLLECTION_TEST):
        dbhandle.createCollection(ROOT_COLLECTION_TEST)
        print("Created test collection at {}".format(ROOT_COLLECTION_TEST))


if __name__ == '__main__':
    # wait until the database is actually up
    maxTimeout = 60
    waitBetween = 5
    count = 0

    while True:
        last_ex = None
        try:
            dbhandle = db.ExistDB(SERVER_URL)
            if dbhandle.hasCollection('/system'):
                break
        except (TimeoutError, HTTPError, ExistDBException) as last_ex:
            if count > maxTimeout//waitBetween:
                msg = "Starting project database timed out! Last exception: {}"
                raise TimeoutError(msg.format(last_ex))

        sleep(waitBetween)
        count += 1

    # Now initialize
    init_local_db(dbhandle)
