__author__ = 'steffen.hauf@xfel.eu'
from contextlib import ContextDecorator
from eulexistdb import db
from .util import assure_running
from .db_settings import db_settings

class ProjectDatabase(ContextDecorator):

    def __enter__(self, user, password, server = None, port = None):
        #get our environment straightened out
        if server is None:
            server = os.getenv('KARABO_PROJECT_DB', None)

        if server is None:
            raise EnvironmentError("No environment variable KARABO_PROJECT_DB"
                                   " found, nor was a server to connect to"
                                   " given!")

        if port is None:
            port = os.getenv('KARABO_PROJECT_DB_PORT', 8080)
            print("Connecting to default port 8080")

        #assure their is a database running where we assume one would be
        assure_running(server, port)

        #now create our actual handle
        settings = db_settings(user, password, server, port)
        self.dbhandle = db.ExistDB(settings.server_url)


    def __exit__(self, exc_type, exc_val, exc_tb):
        #nothing to do as dbhandle does not keep the connection open
        pass



