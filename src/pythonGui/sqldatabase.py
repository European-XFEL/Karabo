#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents a database connection based on SQLite.
"""

__all__ = ["SqlDatabase"]


import globals

from PyQt4.QtCore import QCoreApplication, QDir, QFile
try:
    from PyQt4.QtSql import QSqlDatabase, QSqlQuery
except:
    print "*ERROR* The PyQt4 sql module is not installed"

import os.path
from sys import platform
if "win" in platform:
    from PyQt4.QtGui import QApplication
    from distutils.sysconfig import get_python_lib
    QApplication.addLibraryPath(os.path.join(get_python_lib(), "PyQt4", "plugins"))


class SqlDatabase(QSqlDatabase):


    def __init__(self):
        super(SqlDatabase, self).__init__(QSqlDatabase.addDatabase("QSQLITE"))

        # Use temp path for database stuff
        self.dbName = os.path.join(globals.HIDDEN_KARABO_FOLDER,
                      "xfelgui-{}.db".format(QCoreApplication.applicationPid()))
        #print "database:", self.dbName
        
        # Establish database connection
        self.setDatabaseName(self.dbName)


    def openConnection(self):
        # Called from manager.reset() method
        if self.open():
            #print self.dbName, "connection established."
            query = QSqlQuery(self)
            #query.exec_("PRAGMA foreign_keys = ON;");

            tables = self.tables()
            #if len(tables) < 1:
            #    print "Creating database tables"

            if not "tLog" in tables:
                #print "Creating tLog table..."
                # Create table for event data
                queryText = "CREATE TABLE tLog " \
                              "(id INTEGER PRIMARY KEY, " \
                              "dateTime DATETIME, " \
                              "messageType String(50), " \
                              "instanceId CLOB, " \
                              "description CLOB, " \
                              "additionalDescription CLOB);"
                query.exec_(queryText)
        else:
            print "An error occurred while opening the database connection."


    def closeConnection(self):
        # Called from network.endServerConnection method
        #print self.dbName, "connection closed."
        # Clear database
        #query = QSqlQuery(self)
        #query.exec_("DELETE FROM tLog;")
        
        # Close database
        self.close()
        # Remove database file
        db = QFile(self.dbName)
        if not db.remove():
            print "Database %s could not be deleted properly." % self.dbName
            print "Permission problems might be the reason."

