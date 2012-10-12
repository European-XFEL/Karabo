#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a database connection based on SQLite.
"""

__all__ = ["SqlDatabase"]


from PyQt4.QtCore import *
try:
    from PyQt4.QtSql import QSqlDatabase, QSqlDriver, QSqlQuery, QSqlQueryModel
except:
    print "*ERROR* The PyQt4 sql module is not installed"


class SqlDatabase(object):


    def __init__(self):
        super(SqlDatabase, self).__init__()

        # Use temp path for database stuff
        xfelDir = QDir.tempPath()
        dir = QDir(xfelDir)
        self.__dbName = xfelDir + "/xfelgui-" + str(QCoreApplication.applicationPid()) +".db"
        #print "database:", self.__dbName
        
        #self.__database = QSqlDatabase.addDatabase("QOCI")
        #self.__database.setHostName("131.169.140.93")
        #self.__database.setDatabaseName("XE");
        #self.__database.setPort(1521)
        #self.__database.setUserName("xfel")
        #self.__database.setPassword("xfelpass")
        
        #drivers = QSqlDatabase.drivers()
        #print "Available drivers:"
        #for i in xrange(drivers.count()):
        #    print drivers[i]
        #print ""
        #if not QSqlDatabase.isDriverAvailable("QOCI"):
        #    print "No QOCI driver available"
        
        # Establish database connection
        self.__database = QSqlDatabase.addDatabase("QSQLITE")
        self.__database.setDatabaseName(self.__dbName)

        if self.__database.open():
            print self.__dbName, "opened"
            query = QSqlQuery(self.__database)
            #query.exec_("PRAGMA foreign_keys = ON;");

            tables = self.__database.tables()
            if tables.isEmpty():
                print "Creating database tables"

            if not tables.contains("tLog"):
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

            if not tables.contains("tNode"):
                #print "Creating tNode table..."
                # Create table for event data
                queryText = "CREATE TABLE tNode " \
                              "(id INTEGER PRIMARY KEY, " \
                              "name CLOB);"
                query.exec_(queryText)

            if not tables.contains("tDeviceServerInstance"):
                #print "Creating tDeviceServerInstance table..."
                # Create table for event data
                queryText = "CREATE TABLE tDeviceServerInstance " \
                              "(id INTEGER PRIMARY KEY, " \
                              "nodId INTEGER, " \
                              "name CLOB, " \
                              "status String(50));"
                query.exec_(queryText)

            if not tables.contains("tDeviceClass"):
                #print "Creating tDeviceClass table..."
                # Create table for event data
                queryText = "CREATE TABLE tDeviceClass " \
                              "(id INTEGER PRIMARY KEY, " \
                              "devSerInsId INTEGER, " \
                              "name CLOB, " \
                              "schema CLOB);"
                              #"version String(50));"
                query.exec_(queryText)

            if not tables.contains("tDeviceInstance"):
                #print "Creating tDeviceInstance table..."
                # Create table for event data
                queryText = "CREATE TABLE tDeviceInstance " \
                              "(id INTEGER PRIMARY KEY, " \
                              "devClaId INTEGER, " \
                              "name CLOB, " \
                              "status String(50), " \
                              "schema CLOB);"
                              #"config CLOB);"
                query.exec_(queryText)
        else:
            print "An error occurred while opening the database connection."


    def closeConnection(self):
        print "Close database connection"
        # Clear database
        #query = QSqlQuery(self.__database)
        #query.exec_("DELETE FROM tLog;")
        
        # Close database
        self.__database.close()
        # Remove database file
        db = QFile(self.__dbName)
        if not db.remove():
            print "Database %s could not be deleted properly." % self.__dbName
            print "Permission problems might be the reason."

