#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QAbstractItemModel for a
   hierarchical navigation treeview which is based on a database.
"""

__all__ = ["NavigationHierarchyModel"]


from enums import NavigationItemTypes
from sqltreemodel import SqlTreeModel

from PyQt4.QtCore import *
from PyQt4.QtGui import *

try:
    from PyQt4.QtSql import QSqlQueryModel, QSqlQuery
except:
    print "*ERROR* The PyQt4 sql module is not installed"


class NavigationHierarchyModel(SqlTreeModel):
    
    def __init__(self, parent=None):
        super(NavigationHierarchyModel, self).__init__(parent)
        
        self.__nodeOrder = str()
        self.__devSerInsOrder = str()
        self.__devClaOrder = str()
        self.__devInsOrder = str()

        for i in xrange(4):
            self.appendModel(QSqlQueryModel(self))

        #self.setColumnMapping(0, [1])
        self.setHeaderData(0, Qt.Horizontal, "Hierarchical view", Qt.DisplayRole)
        self.setSort(0, Qt.AscendingOrder)

        #self.updateQueries()


### public ###
    def getSchema(self, level, row):
        schema = self.rawData(level, row, 3).toString()
        return schema


### public overrides ###
    def data(self, index, role=Qt.DisplayRole):
        level = self.levelOf(index)
        row = self.mappedRow(index)

        if role == Qt.DisplayRole:
            value = self.rawData(level, row, self.mappedColumn(index), role)
            return value

        if role == Qt.DecorationRole and index.column() == 0:
            if level == 0:
                return QIcon(":host")
            elif level == 1:
                status = self.rawData(level, row, 3).toString()
                if status == "offline":
                    return QIcon(":no")
                elif status == "starting" or status == "online":
                    return QIcon(":yes")
            elif level == 2:
                return QIcon(":device-class")
            elif level == 3:
                status = self.rawData(level, row, 4).toString()
                if status == "error":
                    return QIcon(":device-instance-error")
                else:
                    return QIcon(":device-instance")

        return QVariant()


### protected overrides ###
    def updateQueries(self):
        #print "updateQueries..."
        
        if self.sortOrder() == Qt.AscendingOrder:
            order = "ASC"
        else:
            order = "DESC"

        if self.sortColumn() == 0:
            self.__nodeOrder = QString("name %1").arg(order)
            self.__devSerInsOrder = QString("d.name %1").arg(order)
            self.__devClaOrder = QString("dc.name %1").arg(order)
            self.__devInsOrder = QString("di.name %1").arg(order)

        self._refresh()


### private ###
    def _refresh(self):
        #print "refresh..."
        
        nodeQuery = "SELECT id, name FROM tNode"
        devSerInsQuery = "SELECT d.id, d.nodId, d.name, d.status FROM tDeviceServerInstance AS d" \
                         " JOIN tNode AS n ON n.id = d.nodId"
        devClaQuery = "SELECT dc.id, dc.devSerInsId, dc.name, dc.schema FROM tDeviceClass AS dc" \
                      " JOIN tDeviceServerInstance AS d ON d.id = dc.devSerInsId AND d.status IS NOT 'offline'"
        devInsQuery = "SELECT di.id, di.devClaId, di.name, di.schema, di.status FROM tDeviceInstance AS di" \
                      " JOIN tDeviceClass AS dc ON dc.id = di.devClaId AND di.status IS NOT 'offline'"
        
        #sqlQuery = QSqlQuery()
        #sqlQuery.prepare(QString("%1 ORDER BY %2").arg(nodeQuery, self.__nodeOrder))
        #sqlQuery.exec_()
        
        # Without sorting...
        #self.modelAt(0).setQuery(nodeQuery)
        #self.modelAt(1).setQuery(devSerInsQuery)
        #self.modelAt(2).setQuery(devClaQuery)
        #self.modelAt(3).setQuery(devInsQuery)
        
        # Use sorting
        self.modelAt(0).setQuery(QString("%1 ORDER BY %2").arg(nodeQuery, self.__nodeOrder))
        self.modelAt(1).setQuery(QString("%1 ORDER BY %2").arg(devSerInsQuery, self.__devSerInsOrder))
        self.modelAt(2).setQuery(QString("%1 ORDER BY %2").arg(devClaQuery, self.__devClaOrder))
        self.modelAt(3).setQuery(QString("%1 ORDER BY %2").arg(devInsQuery, self.__devInsOrder))
        
        self.updateData()


    def insertInto(self, itemInfo):
        #print "insertInto DB", itemInfo
        
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')
        
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        
        type = itemInfo.get(QString('type'))
        if type is None:
            type = itemInfo.get('type')
        
        if type is NavigationItemTypes.NODE:
            # NODE: insert parameter into database
            queryText = "INSERT INTO tNode (id, name) VALUES ('" + str(id) + "', '" + name + "');"
            success = QSqlQuery().exec_(queryText)
            if not success:
                # Table row already exists
                queryText = "REPLACE INTO tNode (id, name) VALUES ('" + str(id) + "', '" + name + "');"
                QSqlQuery().exec_(queryText)
        else:
            # DEVICE_SERVER_INSTANCE, DEVICE_CLASS or DEVICE_INSTANCE
            refType = itemInfo.get(QString('refType'))
            if refType is None:
                refType = itemInfo.get('refType')

            refId = itemInfo.get(QString('refId'))
            if refId is None:
                refId = itemInfo.get('refId')
            
            if type is NavigationItemTypes.DEVICE_SERVER_INSTANCE:
                status = itemInfo.get(QString('status'))
                if status is None:
                    status = itemInfo.get('status')
                
                queryText = "INSERT INTO tDeviceServerInstance (id, name, nodId, status) VALUES ('" + str(id) + "', '" + name + \
                            "', '" + str(refId) + "', '" + status + "');"
                success = QSqlQuery().exec_(queryText)
                if not success:
                    # Table row already exists
                    queryText = "REPLACE INTO tDeviceServerInstance (id, name, nodId, status) VALUES ('" + str(id) + "', '" + name + \
                                "', '" + str(refId) + "', '" + status + "');"
                    QSqlQuery().exec_(queryText)
            elif type is NavigationItemTypes.DEVICE_CLASS:
                schema = itemInfo.get(QString('schema'))
                if schema is None:
                    schema = itemInfo.get('schema')
                
                queryText = "INSERT INTO tDeviceClass (id, name, devSerInsId, schema) VALUES ('" + str(id) + "', '" + name + \
                            "', '" + str(refId) + "', '" + schema + "');"
                success = QSqlQuery().exec_(queryText)
                if not success:
                    # Table row already exists
                    queryText = "REPLACE INTO tDeviceClass (id, name, devSerInsId, schema) VALUES ('" + str(id) + "', '" + name + \
                                "', '" + str(refId) + "', '" + schema + "');"
                    QSqlQuery().exec_(queryText)
            elif type is NavigationItemTypes.DEVICE_INSTANCE:
                status = 'online'
                
                schema = itemInfo.get(QString('schema'))
                if schema is None:
                    schema = itemInfo.get('schema')
                
                queryText = "INSERT INTO tDeviceInstance (id, name, devClaId, status, schema) VALUES ('" + str(id) + "', '" + name + \
                            "', '" + str(refId) + "', '" + status + "', '" + schema + "');"
                success = QSqlQuery().exec_(queryText)
                if not success:
                    # Table row already exists
                    queryText = "REPLACE INTO tDeviceInstance (id, name, devClaId, status, schema) VALUES ('" + str(id) + "', '" + name + \
                                "', '" + str(refId) + "', '" + status + "', '" + schema + "');"  
                    QSqlQuery().exec_(queryText)


    # Triggered not per panel but once from Manager when update needed
    def onUpdateDeviceServerInstance(self, itemInfo):
        #print "updateDeviceServerInstance", itemInfo
        
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')

        status = itemInfo.get(QString('status'))
        if status is None:
            status = itemInfo.get('status')

        sqlQuery = QSqlQuery()
        queryText = "UPDATE tDeviceServerInstance SET status='" + status + "' WHERE id=" + str(id) + ";"
        sqlQuery.exec_(queryText)
        
        if status == 'offline':
            # Update DEVICE_INSTANCE
            queryText = "SELECT id FROM tDeviceClass WHERE devSerInsId="+ str(id) +";"
            sqlQuery.exec_(queryText)
            while sqlQuery.next():
                devClaId = sqlQuery.value(0).toString()
                sqlQueryUpdate = QSqlQuery()
                queryText = "UPDATE tDeviceInstance SET status='" + status + "' WHERE devClaId=" + str(devClaId) + ";"
                sqlQueryUpdate.exec_(queryText)
        
        # Update view with model...
        self.updateQueries()


    # Triggered not per panel but once from Manager when update needed
    def onUpdateDeviceInstance(self, itemInfo):
        #print "updateDeviceInstance", itemInfo
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')
        
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        
        #status = 'offline'
        #sqlQuery = QSqlQuery()
        #queryText = "UPDATE tDeviceInstance SET status='" + status + "' WHERE id=" + str(id) + ";"
        #sqlQuery.exec_(queryText)
        
        # Remove completely from database
        sqlQuery = QSqlQuery()
        queryText = "DELETE from tDeviceInstance WHERE id=" + str(id) + ";"
        sqlQuery.exec_(queryText)

        # Update view with model...
        self.updateQueries()


    def updateDeviceInstanceSchema(self, instanceId, schema):
        #print "updateDeviceInstanceSchema", instanceId
        sqlQuery = QSqlQuery()
        queryText = "UPDATE tDeviceInstance SET schema='" + schema + "' WHERE name='" + str(instanceId) + "';"
        sqlQuery.exec_(queryText)

        # Update view with model...
        self.updateQueries()


    def updateErrorState(self, instanceId, hasError):
        #print "updateErrorState", instanceId, hasError
        if hasError:
            status = 'error'
        else:
            status = 'online'
        
        sqlQuery = QSqlQuery()
        queryText = "SELECT id from tDeviceInstance WHERE name='" + str(instanceId) + "' AND status='online';"
        sqlQuery.exec_(queryText)
        
        while sqlQuery.next():
            id = sqlQuery.value(0).toString()
            sqlQueryUpdate = QSqlQuery()
            queryText = "UPDATE tDeviceInstance SET status='" + status + "' WHERE id=" + str(id) + ";"
            sqlQueryUpdate.exec_(queryText)

        # Update view with model...
        self.updateQueries()

