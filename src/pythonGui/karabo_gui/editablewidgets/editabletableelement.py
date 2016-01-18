#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class EditableWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["EditableTableElement"]


from util import SignalBlocker
from widget import DisplayWidget, EditableWidget

from karabo.api_2 import Hash, Type, VectorHash

from PyQt4.QtGui import (QTableView, QAbstractItemView, QMenu, QDialog, QComboBox,
                        QVBoxLayout, QWidget, QDialogButtonBox, QCheckBox)
from PyQt4.QtCore import *

import karabo_gui.icons as icons

import manager

class TableModel(QAbstractTableModel):
    def __init__(self, columnSchema, editingFinished, parent=None, *args):
        super(QAbstractTableModel,self).__init__(parent, *args)
        self.columnHash = columnSchema.hash
        self.columnSchema = columnSchema
        self.data = []
        self.connectedMonitors = {}
        self.connectedMonitorsByCell = {}
        self.editingFinished = editingFinished
        self.role = Qt.EditRole
        #connect so that monitors recieve updates:
        #manager.Manager().signalUpdateScenes.connect(self.monitorChanged)
        
    def setRole(self, role):
        self.role = role

    def rowCount(self, parent):
        return len(self.data)
    
    def columnCount(self, parent):
        return len(self.columnHash)
    
    def data(self, idx, role):
        """"""
        if not idx.isValid():
            return None
            #return QVariant()
        if idx.row() < 0 or idx.row() >= len(self.data):
            return None
        
        if idx.column() < 0  or idx.column() >= len(self.columnHash):
            return None
            #return QVariant()
        
        if role == Qt.DisplayRole:
            row = self.data[idx.row()]
            columnKey = self.columnHash.getKeys()[idx.column()]
            
            value = None
            if self.data[idx.row()].hasAttribute(columnKey, "isAliasing") and self.role == Qt.EditRole:
                value = "="+self.data[idx.row()].getAttribute(columnKey, "isAliasing")
            else:
                value = row[columnKey]
            #
            #valueType = self.columnHash.getAttribute(columnKey, "valueType")
            return str(value)
        
        if role == Qt.DecorationRole:
            columnKey = self.columnHash.getKeys()[idx.column()]
            if (self.data[idx.row()].hasAttribute(columnKey, "isAliasing") and 
                    self.role == Qt.DisplayRole):
                        
                monitoredDeviceId = (self.data[idx.row()]
                    .getAttribute(columnKey, "isAliasing").split(".")[0])
                status =  manager.getDevice(monitoredDeviceId).status
                if status in ["monitoring", "alive"]:
                    return icons.tableOnline.pixmap(10,10)
                elif status in ["online", "requested", "schema"]:
                    return icons.tablePending.pixmap(10,10)
                else:
                    return icons.tableOffline.pixmap(10,10)
    
    
        return None
    
    
    def headerData(self, section, orientation, role):
        if role != Qt.DisplayRole:
            return None
        
        if orientation == Qt.Horizontal:
            if section >= len(self.columnHash):
                return None
            columnKey = self.columnHash.getKeys()[section]
            if self.columnHash.hasAttribute(columnKey, "displayedName"):
             return self.columnHash.getAttribute(columnKey, "displayedName")
            return columnKey
        
        if orientation == Qt.Vertical:
            return str(section)
        
        return None
        
    def flags(self, idx):
        """"""
        if not idx.isValid():
            return Qt.ItemIsEnabled
        
        return QAbstractTableModel.flags(self, idx) | Qt.ItemIsEditable
    
    def _removeMonitor(self, row, col, role):
        if "{}.{}".format(row,col) in self.connectedMonitorsByCell:
            cKey = self.columnHash.getKeys()[col]
            resp = self.connectedMonitorsByCell["{}.{}".format(row,col)]
            del self.connectedMonitorsByCell["{}.{}".format(row,col)]
            
            #need to remove attribute
            item = Hash()
            for k,v,a in self.data[row].iterall():
                item[k] = v
                for aa in a:
                    if aa != "isAliasing":
                        item.setAttribute(k, self.data[row].getAttribute(k, aa))
            self.data[row] = item
            
            self.connectedMonitors[resp].remove((row,col))
            if len(self.connectedMonitors[resp]) == 0:
                del self.connectedMonitors[resp]
                deviceId = resp.split(".")[0]
                deviceProperty = ".".join(resp.split(".")[1:])
                box = manager.getDevice(deviceId).getBox(deviceProperty.split("."))
                if role == Qt.DisplayRole:
                    box.signalUpdateComponent.disconnect(self.monitorChanged)  
                
    def _addMonitor(self, row, col, resp, role):
        cKey = self.columnHash.getKeys()[col]
        deviceId = resp.split(".")[0]
        deviceProperty = ".".join(resp.split(".")[1:])
        box = manager.getDevice(deviceId).getBox(deviceProperty.split("."))
        
        
        #set these as attributes cell
        self.data[row].setAttribute(cKey, "isAliasing", resp)
        if resp not in self.connectedMonitors:
            self.connectedMonitors[resp] = [(row,col)]
            if role == Qt.DisplayRole:
                
                box.signalUpdateComponent.connect(self.monitorChanged)
        else:
            self.connectedMonitors[resp].append((row,col))

        self.connectedMonitorsByCell["{}.{}".format(row,col)] = resp
        return box.value
        
        
    def setData(self, idx, value, role, isAliasing = None, fromValueChanged = False):
        """"""
        
        if idx.isValid() and role == Qt.EditRole or role == Qt.DisplayRole:
            row = idx.row()
            col = idx.column()
            
            if row < 0 or row >= len(self.data):
                return False
            
            if col < 0 or col >= len(self.columnHash):
                return False
            
            
            
            
            cKey = self.columnHash.getKeys()[col]
            valueType = self.columnSchema.getValueType(cKey)()
            
            self._removeMonitor(row, col, role)
            #handle monitor requests from editor
            if(isinstance(value, str)) and role == Qt.EditRole:
                #remove monitor if one exists
                
                if "=" in value and not fromValueChanged:
                    resp = value[1:]
                    value = self._addMonitor(row, col, resp, role)
                elif fromValueChanged and isAliasing != None:
                    value = self._addMonitor(row, col, isAliasing, role)
                    
            elif role == Qt.EditRole and not fromValueChanged:
                 #remove monitor if one exists
                self._removeMonitor(row, col, role)
            
            #initiate monitors if displaying
            if role == Qt.DisplayRole:
                
                if isAliasing != None:
                    value = self._addMonitor(row, col, isAliasing, role)
                
                    
            #now display value
            try:
                value = valueType.cast(value)
            except:
                value = None
            self.data[row][cKey] = value
            
            self.dataChanged.emit(idx,idx)
            if role == Qt.EditRole and not fromValueChanged:
                self.editingFinished(self.data)
            return True
        
        return False
    
    def monitorChanged(self, box, value, timestamp=None ):
        key = box.key()
        affectedCells = self.connectedMonitors[key]
        for c in affectedCells:
            cKey = self.columnHash.getKeys()[c[1]]
            valueType = self.columnSchema.getValueType(cKey)()
            value = valueType.cast(value)
            self.data[c[0]][cKey] = value
            idx = self.index(c[0], c[1], QModelIndex())
            self.dataChanged.emit(idx,idx)
            
            #self.setData(idx, value, Qt.DisplayRole)
        #for k in self.connectedMonitors:
        #    r, c = k.split(".")
        #    deviceId, deviceProperty = self.connectedMonitors[k]
        #    val = manager.getDevice(deviceId).getBox(deviceProperty.split(".")).value
        #    print(val)
            
        
    def insertRows(self, pos, rows, idx):
        """"""
        self.beginInsertRows(QModelIndex(), pos, pos+rows-1)
        
        for r in range(rows):
            rowHash = Hash()
            for key in self.columnHash.getKeys():
                val = None
                if self.columnHash.hasAttribute(key, "defaultValue"):
                    val = self.columnHash.getAttribute(key, "defaultValue")
                    #if not isinstance(val, str):
                    #    val = str(val)
                try:
                    valueType = self.columnSchema.getValueType(key)()
                    val = valueType.cast(val)
                except:
                    pass
                rowHash[key] = val
            
            self.data.insert(pos+r, rowHash)
        
        self.endInsertRows()
        
        return True
        
    def removeRows(self, pos, rows, idx):
        """"""
        self.beginRemoveRows(QModelIndex(), pos, pos+rows-1)
        
        for r in range(rows):
            self.data.pop(pos+r)
        
        self.endRemoveRows()
        return True
    
    def getHashList(self):
        return self.data
    
    def setHashList(self, data):
        self.data = data




class FromPropertyPopUp(QDialog):
    
    def __init__(self):
        super(QWidget, self).__init__()
        
        self.selectedDeviceId = None
        self.selectedProperty = None
        
        self.layout = QVBoxLayout(self)
        
        
        self.propertyCombo = QComboBox()
        self.propertyCombo.setEditable(True)
        self.propertyCombo.currentIndexChanged['QString'].connect(self.propertySelectionChanged)
        
        
        self.deviceCombo = QComboBox()
        self.deviceCombo.setEditable(True)
        availableDevices = self.getCurrentDeviceInstances()
        self.deviceCombo.addItems(availableDevices)
        if len(availableDevices) > 0:
            self.selectedDeviceId = availableDevices[0]
            self.deviceIdSelectionChanged(availableDevices[0])
        self.deviceCombo.currentIndexChanged['QString'].connect(self.deviceIdSelectionChanged)
        
        #connect signal if device schema not yet available
        manager.Manager().signalUpdateScenes.connect(self.delayedSchema)
        
        self.actAsMonitorCheck = QCheckBox("Act as monitor")
        self.actAsMonitorCheck.setChecked(True)
       
        
        #buttons for return
        self.buttonBox = QDialogButtonBox(self)
        self.buttonBox.setOrientation(Qt.Horizontal)
        self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
        QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
        QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
        
        self.layout.addWidget(self.deviceCombo)
        self.layout.addWidget(self.propertyCombo)
        self.layout.addWidget(self.actAsMonitorCheck)
        self.layout.addWidget(self.buttonBox)
        
        
        
    def getCurrentDeviceInstances(self):
        devicesHash = manager.Manager().systemHash["device"]
        devices = []
        for k, v, a in devicesHash.iterall():
            if 'type' in a:
                if a['type'] == 'device' and "Gui" not in k and "Log" not in k and "ProjectManager" not in k:
                    devices.append(k)
        return devices
    
    def deviceIdSelectionChanged(self, deviceId):
        self.propertyCombo.clear()
        self.selectedDeviceId = deviceId
       
        
        descriptor = manager.getDevice(deviceId).descriptor
        
        if descriptor is not None:
            properties = []
            for i, v in descriptor.dict.items():
                if isinstance(v, Type) and not isinstance(v, VectorHash):
                #the latter won't work as it would result in a non-flat table
                    properties.append(i)
            
            self.propertyCombo.addItems(properties)
        
            
            
    def delayedSchema(self):
        if self.propertyCombo.count() == 0:
            self.deviceIdSelectionChanged(self.selectedDeviceId)
            
    def propertySelectionChanged(self, property):
        self.selectedProperty = property
        
    def getValues(self):
        return self.selectedDeviceId, self.selectedProperty, self.actAsMonitorCheck.isChecked()


class EditableTableElement(EditableWidget, DisplayWidget):
    category = VectorHash
    priority = 100
    alias = "Table Element"

    def __init__(self, box, parent, role = Qt.EditRole):
        super(EditableTableElement, self).__init__(box)
        
        self.role = role
        self.columnSchema = getattr(box.descriptor, "rowSchema")
        self.columnHash = self.columnSchema.hash
        self.tableModel = TableModel(self.columnSchema, self.onEditingFinished)
        self.tableModel.setRole(role)
       
        self.widget = QTableView()
        self.widget.setModel(self.tableModel)
        self.widget.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.widget.horizontalHeader().setStretchLastSection(True)
        self.widget.verticalHeader()
        if role == Qt.EditRole:
            self.widget.setEditTriggers(QAbstractItemView.SelectedClicked)
        else:
            self.widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.widget.setSelectionMode(QAbstractItemView.SingleSelection)
        
        #add context menu to cells
        self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.widget.customContextMenuRequested.connect(self.cellPopUp)
        
        #add context menu to headers to add and remove rows
        self.leftTableHeader = self.widget.verticalHeader()
        if role == Qt.EditRole:
            self.leftTableHeader.setContextMenuPolicy(Qt.CustomContextMenu)
            self.leftTableHeader.customContextMenuRequested.connect(self.headerPopUp)
        
        
        self.tableModel.insertRows(0, 1, QModelIndex())
           
            
    @classmethod
    def isCompatible(cls, box, readonly):
        return hasattr(box.descriptor, "rowSchema")

    @property
    def value(self):
        ret = VectorHash()
        ret = ret.cast(self.tableModel.getHashList())
        return ret


    def valueChanged(self, box, value, timestamp=None):
      
        if value is None:
            return
        
        if self.tableModel.rowCount(None) > len(value):
            self.tableModel.removeRows(len(value)-1,
                self.tableModel.rowCount(None)-len(value), QModelIndex())
        
        
        #add rows if necessary
        if self.tableModel.rowCount(None) < len(value):
            self.tableModel.insertRows(self.tableModel.rowCount(None),
                len(value)- self.tableModel.rowCount(None), QModelIndex())
            
        for r, row in enumerate(value):
            ckeys = row.getKeys()
            for c, col in enumerate(ckeys):
                idx = self.tableModel.index(r, c, QModelIndex())
                isAliasing = None
                if row.hasAttribute(col, "isAliasing"):
                    isAliasing = row.getAttribute(col, "isAliasing")
                self.tableModel.setData(idx, row[col], self.role, isAliasing,
                    fromValueChanged = True)
      


    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, value)
        
        
    def headerPopUp(self, pos):
        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i
        
        menu = QMenu()
        addAction = menu.addAction("Add Row")
        removeAction = menu.addAction("Remove Row")
        action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
        if action == addAction and idx != None:
            self.tableModel.insertRows(idx.row(), 1, QModelIndex())
            
        if action == removeAction and idx != None:
            self.tableModel.removeRows(idx.row(), 1, QModelIndex())
            
            
    def cellPopUp(self, pos):
        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i
        
        menu = QMenu()
        
        #action to pop up from property field
        setFromPropertyAction = None
        if self.role == Qt.EditRole:
            setFromPropertyAction = menu.addAction("From device property")
        
        
        #check if this cell can be set to a default value
        col = idx.column()
    
        setDefaultAction = None
        #setDefaultAction.setEnabled(False)
        cKey = None
        if col >= 0 and col < len(self.columnHash):
            cKey = self.columnHash.getKeys()[col]
            
            if self.columnHash.hasAttribute(cKey, "defaultValue") and self.role == Qt.EditRole:
                setDefaultAction = menu.addAction("Set to Default")
                
                
            #add a hint to the object type
            typeDummyAction = menu.addAction(self.columnHash.getAttribute(cKey, "valueType"))
            typeDummyAction.setEnabled(False)
                
        action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
        if action == setDefaultAction and cKey != None and setDefaultAction != None:
            defaultValue = self.columnHash.getAttribute(cKey, "defaultValue")
            self.tableModel.setData(idx, defaultValue, Qt.EditRole)
            
        if action == setFromPropertyAction and setFromPropertyAction != None:
            propertyPopUp = FromPropertyPopUp()
            if propertyPopUp.exec_():
                deviceId, deviceProperty, isMonitor = propertyPopUp.getValues()
                if isMonitor:
                    value = "={}.{}".format(deviceId, deviceProperty)
                else:
                    value = "{}.{}".format(deviceId, deviceProperty)
                self.tableModel.setData(idx, value, Qt.EditRole)
            
            

    def copy(self, item):
        copyWidget = EditableTableElement(item=item)

        copyWidget.tableModel.setHashList(self.tableModel.getHashList())

        return copyWidget
    
    
    def setReadOnly(self, ro):
        if ro:
            self.role = Qt.DisplayRole