__author__ = 'steffen.hauf@xfel.eu'

class db_settings:
    user = 'admin'
    password = 'karabo'
    server_uri = "localhost:8080/exist/xmlrpc"
    server_url = "http://{}:{}@{}".format(user,password,server_uri )
    root_collection = "/krb_config"
    root_collection_test = "/krb_test"