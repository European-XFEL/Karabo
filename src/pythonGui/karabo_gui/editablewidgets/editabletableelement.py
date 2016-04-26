#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents a widget plugin for tables and 
is created as a composition of EditableWidget and DisplayWidget. Rendering in 
read-only mode is controlled via the set readOnly method.

The element is applicable for VECTOR_HASH data types, which have a "rowSchema" 
attribute. The rowSchema is a Hash (schema.parameterHash to be precise), which
defines the column layout, i.e. column count, column data types and column headers.

In case the rowSchema contains a "displayedName" field this is used as the 
column header, otherwise the field's key is used.

For string fields with options supplied the cell is rendered as a drop down menu.
Boolean fields are rendered as check boxes.

Additional manipulation functionality includes, adding, deleting and duplicating
rows (the latter require a cell or row to be selected).

A right-click will display the cells data type both in Display and Edit mode. In
edit mode additionally a pop-up menue is available which allows selection of
device properties from instanciated devices. In this case the cell will "mirror"
the selected propertiy and the field in the underlying Hash of this row will have 
an "isAliasing=PARAM_PATH" added. This should be evaluated on device side whenever
the current value of the parameter is needed.

The Table widget supports drag and drop of deviceId's from the navigation and 
project panel. Dropping on a string cell will replace the string with the deviceId.
Dropping on a non-string cell or on an empty region will add a row in which the
first string-type column encountered is pre-filled with the deviceID.


"""

__all__ = ["EditableTableElement"]



from karabo_gui.widget import DisplayWidget, EditableWidget

from karabo.api_2 import Hash, Type, VectorHash, SchemaHashType
import karabo_gui.icons as icons
from karabo_gui.enums import NavigationItemTypes
from karabo_gui.const import ns_karabo
from karabo_gui.topology import getDevice, Manager

import schema

from PyQt4.QtGui import (QTableView, QAbstractItemView, QMenu, QDialog, QComboBox,
                        QVBoxLayout, QWidget, QDialogButtonBox, QCheckBox,
                        QItemDelegate, QStyledItemDelegate, QSizePolicy)
from PyQt4.QtCore import (Qt, QAbstractTableModel, QModelIndex, QObject,
                          SIGNAL, SLOT, pyqtSlot)

import copy


class TableModel(QAbstractTableModel):
    def __init__(self, columnSchema, editingFinished, parent=None, *args):
        super(QAbstractTableModel,self).__init__(parent, *args)
        
        self.columnSchema = columnSchema
        self.columnHash = self.columnSchema.hash if self.columnSchema is not None else Hash()
        self.cdata = []
        self.connectedMonitors = {}
        self.connectedMonitorsByCell = {}
        self.editingFinished = editingFinished
        self.role = Qt.EditRole
        #connect so that monitors recieve updates:
        #Manager().signalUpdateScenes.connect(self.monitorChanged)
        
    def setRole(self, role):
        self.role = role

    def rowCount(self, parent):
        return len(self.cdata)
    
    def columnCount(self, parent):
        return len(self.columnHash)
    
    def data(self, idx, role):
        """"""
        if not idx.isValid():
            return None
            #return QVariant()
        if idx.row() < 0 or idx.row() >= len(self.cdata):
            return None
        
        if idx.column() < 0  or idx.column() >= len(self.columnHash):
            return None
            #return QVariant()
            
        if role == Qt.CheckStateRole and self.role == Qt.EditRole:
            row = self.cdata[idx.row()]
            columnKey = self.columnHash.getKeys()[idx.column()]
            value = row[columnKey]
            valueType = self.columnSchema.getValueType(columnKey)() 
            if isinstance(valueType, schema.Bool):
                return Qt.Checked if value == True else Qt.Unchecked
        
        if role == Qt.DisplayRole or role == Qt.EditRole:
            row = self.cdata[idx.row()]
            columnKey = self.columnHash.getKeys()[idx.column()]
            
            value = None
            if self.cdata[idx.row()].hasAttribute(columnKey, "isAliasing") and role == Qt.EditRole:
                value = "="+self.cdata[idx.row()].getAttribute(columnKey, "isAliasing")
            else:
                value = row[columnKey]
            #
            #valueType = self.columnHash.getAttribute(columnKey, "valueType")
            return str(value)
        
        if role == Qt.DecorationRole:
            columnKey = self.columnHash.getKeys()[idx.column()]
            if (self.cdata[idx.row()].hasAttribute(columnKey, "isAliasing")): #and 
                    #self.role == Qt.DisplayRole):
                        
                monitoredDeviceId = (self.cdata[idx.row()]
                    .getAttribute(columnKey, "isAliasing").split(".")[0])
                status =  getDevice(monitoredDeviceId).status
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
        
        cKey = self.columnHash.getKeys()[idx.column()]
        valueType = self.columnSchema.getValueType(cKey)() 
        if isinstance(valueType, schema.Bool) and self.role == Qt.EditRole:
           
            return Qt.ItemIsUserCheckable | Qt.ItemIsSelectable | Qt.ItemIsEnabled
        
        return QAbstractTableModel.flags(self, idx) | Qt.ItemIsEditable
    
    def _removeMonitor(self, row, col, role):
        if "{}.{}".format(row,col) in self.connectedMonitorsByCell:
            cKey = self.columnHash.getKeys()[col]
            resp = self.connectedMonitorsByCell["{}.{}".format(row,col)]
            del self.connectedMonitorsByCell["{}.{}".format(row,col)]
            
            #need to remove attribute
            item = Hash()
            for k,v,a in self.cdata[row].iterall():
                item[k] = v
                for aa in a:
                    
                    if aa != "isAliasing" or k != cKey:
                        item.setAttribute(k, aa, self.cdata[row].getAttribute(k, aa))
            self.cdata[row] = item
            
            self.connectedMonitors[resp].remove((row,col))
            if len(self.connectedMonitors[resp]) == 0:
                del self.connectedMonitors[resp]
                deviceId = resp.split(".")[0]
                deviceProperty = ".".join(resp.split(".")[1:])
                box = getDevice(deviceId).getBox(deviceProperty.split("."))
                if role == Qt.DisplayRole:
                    box.signalUpdateComponent.disconnect(self.monitorChanged)  
                
    def _addMonitor(self, row, col, resp, role):
        cKey = self.columnHash.getKeys()[col]
        deviceId = resp.split(".")[0]
        deviceProperty = ".".join(resp.split(".")[1:])
        box = getDevice(deviceId).getBox(deviceProperty.split("."))
        
        
        #set these as attributes cell
        self.cdata[row].setAttribute(cKey, "isAliasing", resp)
        if resp not in self.connectedMonitors:
            self.connectedMonitors[resp] = [(row,col)]
            if role == Qt.DisplayRole:
                
                box.signalUpdateComponent.connect(self.monitorChanged)
        elif "{}.{}".format(row,col) in self.connectedMonitorsByCell:
            return box.value
        else:
            self.connectedMonitors[resp].append((row,col))

        self.connectedMonitorsByCell["{}.{}".format(row,col)] = resp
        return box.value
        
        
    def setData(self, idx, value, role, isAliasing = None, fromValueChanged = False):
        """"""
        
        if not idx.isValid():
            return False
        
       
        
        if role == Qt.CheckStateRole:
            
            row = idx.row()
            
            columnKey = self.columnHash.getKeys()[idx.column()]
            valueType = self.columnSchema.getValueType(columnKey)() 
            if isinstance(valueType, schema.Bool):
                
                value =  True if value == Qt.Checked else False
                self.cdata[row][columnKey] = value
                self.dataChanged.emit(idx,idx)
                return True
        
        if role == Qt.EditRole or role == Qt.DisplayRole:
            row = idx.row()
            col = idx.column()
            
            if row < 0 or row >= len(self.cdata):
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
                    
            #elif role == Qt.EditRole and not fromValueChanged:
                #remove monitor if one exists
            #    self._removeMonitor(row, col, role)
            
                
            if isAliasing != None:
               
                value = self._addMonitor(row, col, isAliasing, role)
                
                    
            #now display value
            try:
                value = valueType.cast(value)
            except:
                value = None
                
           
                
                
            self.cdata[row][cKey] = value
            
            self.dataChanged.emit(idx,idx)
            if role == Qt.EditRole and not fromValueChanged:
                self.editingFinished(self.cdata)
            return True
        
        return False
    
    def monitorChanged(self, box, value, timestamp=None ):
        key = box.key()
        affectedCells = self.connectedMonitors[key]
        for c in affectedCells:
            cKey = self.columnHash.getKeys()[c[1]]
            valueType = self.columnSchema.getValueType(cKey)()
            value = valueType.cast(value)
            self.cdata[c[0]][cKey] = value
            idx = self.index(c[0], c[1], QModelIndex())
            self.dataChanged.emit(idx,idx)
            
            #self.setData(idx, value, Qt.DisplayRole)
        #for k in self.connectedMonitors:
        #    r, c = k.split(".")
        #    deviceId, deviceProperty = self.connectedMonitors[k]
        #    val = getDevice(deviceId).getBox(deviceProperty.split(".")).value
        #    print(val)
            
        
    def insertRows(self, pos, rows, idx, iRowHash = None):
        """"""
        self.beginInsertRows(QModelIndex(), pos, pos+rows-1)
        
        for r in range(rows):
            rowHash = copy.copy(iRowHash)
            if rowHash is None:
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
            if pos+r < len(self.cdata):
                self.cdata.insert(pos+r, rowHash)
            else:
                self.cdata.append(rowHash)
        
        self.endInsertRows()
        self.editingFinished(self.cdata)
        
        return True
        
    def removeRows(self, pos, rows, idx):
        """"""
        self.beginRemoveRows(QModelIndex(), pos, pos+rows-1)
        
        for r in range(rows):
            self.cdata.pop(pos+r)
        
        self.endRemoveRows()
        self.editingFinished(self.cdata)
        return True
    
    
    
    def duplicateRow(self, pos):
        
        self.insertRows(pos+1, 1, QModelIndex(), self.cdata[pos])
    
    def getHashList(self):
        return self.cdata
    
    def setHashList(self, data):
        self.cdata = data




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
        Manager().signalUpdateScenes.connect(self.delayedSchema)
        
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
        devicesHash = Manager().systemHash["device"]
        devices = []
        for k, v, a in devicesHash.iterall():
            if 'type' in a:
                if a['type'] == 'device' and "Gui" not in k and "Log" not in k and "ProjectManager" not in k:
                    devices.append(k)
        return devices
    
    def deviceIdSelectionChanged(self, deviceId):
        self.propertyCombo.clear()
        self.selectedDeviceId = deviceId
       
        
        descriptor = getDevice(deviceId).descriptor
        
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

#from http://stackoverflow.com/questions/17615997/pyqt-how-to-set-qcombobox-in-a-table-view-using-qitemdelegate
class ComboBoxDelegate(QItemDelegate):
    
    def __init__(self, options, parent=None, *args):
        super(ComboBoxDelegate, self).__init__(parent, *args)
        self.options = options
        
        
    def createEditor(self, parent, option, index):
        combo = QComboBox(parent)
        combo.addItems(self.options)
        self.connect(combo, SIGNAL("currentIndexChanged(int)"), 
                     self, SLOT("currentIndexChanged()"))
        return combo
        
    def setEditorData(self, editor, index):
        editor.blockSignals(True)
        selection = index.model().data(index, Qt.DisplayRole)
        
        editor.setCurrentIndex(self.options.index(selection))
        editor.blockSignals(False)

    @pyqtSlot()
    def currentIndexChanged(self):
        self.commitData.emit(self.sender())
        
    def setModelData(self, editor, model, index):
        
        model.setData(index, self.options[editor.currentIndex()], Qt.EditRole)
        
        
class KaraboTableView(QTableView):
    def __init__(self, columnSchema,  *args):
        super(KaraboTableView, self).__init__(*args)
        
        self.columnSchema  = columnSchema
        self.cHash = self.columnSchema.hash if self.columnSchema is not None else Hash()
        self.cKeys = self.cHash.getKeys()
        self.firstStringColumn = None
        #self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding)
        
        
        
        for c, cKey in enumerate(self.cKeys):
            valueType = self.columnSchema.getValueType(self.cKeys[c])()
            if isinstance(valueType, schema.String):
                self.firstStringColumn = c
                break
                
      
        
        
    def checkAcceptance(self, e):
        if e.mimeData().data("sourceType") == "NavigationTreeView" or \
           e.mimeData().data("sourceType") == "internal":
            idx = self.indexAt(e.pos())
            
            
            
            valueType = self.columnSchema.getValueType(self.cKeys[idx.column()])()
            source = e.source() 
            itemInfo = source.indexInfo(source.currentIndex())
            type = itemInfo.get('type')
            isDeviceFromProject = 'deviceId' in itemInfo and \
                itemInfo.get('deviceId') != ""
       
            #drop in empty area is also okay but must trigger newRow
            if not idx.isValid() and (type == NavigationItemTypes.DEVICE \
                or isDeviceFromProject):
                e.accept()
                return True, idx, True
            
           
            if not isinstance(valueType, schema.String) and (type == NavigationItemTypes.DEVICE \
                or isDeviceFromProject):
                e.accept()
                return True, idx, True

            #drop onto existing cell
            if isinstance(valueType, schema.String) and \
                (type == NavigationItemTypes.DEVICE or isDeviceFromProject):
                e.accept()
                return True, idx, False
            
            
            
            
            
        e.ignore()
        return False, None, None
        
    def dragEnterEvent(self, e):
        self.checkAcceptance(e)
        
    def dragMoveEvent(self, e):
        self.checkAcceptance(e)
        
    def dropEvent(self, e):
        acceptable, index, newRow = self.checkAcceptance(e)
        
        if acceptable:
            source = e.source()

            itemInfo = source.indexInfo(source.currentIndex())
            deviceId = itemInfo.get('deviceId')
            
            if newRow:
                if self.firstStringColumn is not None:
                    self.model().insertRows(self.model().rowCount(None), 1, QModelIndex())
                    index = self.model().index(self.model().rowCount(None)-1, self.firstStringColumn, QModelIndex())
                    
                    #scroll to the end and pad with new whitespace to drop next item
                    self.scrollToBottom();
                    
                else:
                    return
                
            
            self.model().setData(index, deviceId, Qt.EditRole)
            
           
            
            
            
    

class EditableTableElement(EditableWidget, DisplayWidget):
    category = VectorHash
    priority = 100
    alias = "Table Element"

    def __init__(self, box, parent, role = Qt.EditRole):
        super(EditableTableElement, self).__init__(box)
        
        self.role = role 
        self.columnSchema = None
        if hasattr(box.descriptor, "rowSchema"):
            self.columnSchema = getattr(box.descriptor, "rowSchema")
            self.columnHash = self.columnSchema.hash
        else:
            self.columnHash = Hash()
        
        
        self.tableModel = TableModel(self.columnSchema, self.onEditingFinished)
        self.tableModel.setRole(self.role)
        
        self.widget = KaraboTableView(self.columnSchema)
        self.widget.setModel(self.tableModel)
        self.widget.setSelectionBehavior(QAbstractItemView.SelectItems | \
                QAbstractItemView.SelectRows | QAbstractItemView.SelectColumns)
        self.widget.horizontalHeader().setStretchLastSection(True)
        self.widget.verticalHeader()
        
        if self.role == Qt.EditRole:
            self.widget.setEditTriggers(QAbstractItemView.SelectedClicked)
            self.widget.setAcceptDrops(True)
        else:
            self.widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
        
        self.widget.setSelectionMode(QAbstractItemView.SingleSelection)
        
        #add context menu to cells
        self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.widget.customContextMenuRequested.connect(self.cellPopUp)
        
        #add context menu to headers to add and remove rows
        self.leftTableHeader = self.widget.verticalHeader()
        self.colsWithCombos = []
        if self.role == Qt.EditRole:
            self.leftTableHeader.setContextMenuPolicy(Qt.CustomContextMenu)
            self.leftTableHeader.customContextMenuRequested.connect(self._headerPopUp)
 
            self._setComboBoxes(False)
       
        

   
        
        
  

    def _setComboBoxes(self, ro):
        if self.columnSchema is None:
            return 
        
        if ro:
            
            #remove any combo delegate
            cHash = self.columnSchema.hash
            cKeys = self.columnHash.getKeys()
            self.colsWithCombos = []
            for col, cKey in enumerate(cKeys):
                
                if cHash.hasAttribute(cKey, "options"):
                    delegate = QStyledItemDelegate()
                    self.widget.setItemDelegateForColumn(col, delegate)
                    
        else:
            
            self.leftTableHeader.setContextMenuPolicy(Qt.CustomContextMenu)
            self.leftTableHeader.customContextMenuRequested.connect(self._headerPopUp)
 
            cHash = self.columnSchema.hash
            cKeys = self.columnHash.getKeys()
            self.colsWithCombos = []
            for col, cKey in enumerate(cKeys):
                
                if cHash.hasAttribute(cKey, "options"):
                    delegate = ComboBoxDelegate(cHash.getAttribute(cKey, "options"))
                    self.widget.setItemDelegateForColumn(col, delegate)
                    self.colsWithCombos.append(col)
            
                    
            self._updateComboBoxes()
                    
    def _updateComboBoxes(self):
        if self.role == Qt.EditRole:
            for row in range(0, self.tableModel.rowCount(None)):
                for col in self.colsWithCombos:
                    self.widget.openPersistentEditor(self.tableModel.index(row, col))
            
    @classmethod
    def isCompatible(cls, box, readonly):
        return hasattr(box.descriptor, "rowSchema")

    @property
    def value(self):
        ret = VectorHash()
        ret = ret.cast(self.tableModel.getHashList())
        return ret



    def save(self, element):
        element.set(ns_karabo + "columnSchema", SchemaHashType.toString(self.columnSchema))
        

    def load(self, element):
        try:
            if self.columnSchema is None:
                
                self.columnSchema = SchemaHashType.fromstring(element.get(ns_karabo + "columnSchema"))
                self.columnHash = self.columnSchema.hash
                self.tableModel = TableModel(self.columnSchema, self.onEditingFinished)
                self.tableModel.setRole(self.role)
                self.widget.setModel(self.tableModel)
                self._setComboBoxes(self.role == Qt.DisplayRole)
                
        except AttributeError:
            print("Loading column schema failed. Maybe this is an older project"+ \
                  "file. Try saving the project and reloading.")
            pass
            
        
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
       
        self._updateComboBoxes()
      


    def onEditingFinished(self, value):
        self._updateComboBoxes()
        EditableWidget.onEditingFinished(self, value)
        
        
        
    def _headerPopUp(self, pos):
        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i
            
       
            
        menu = QMenu()
        if idx != None:
            addAction = menu.addAction("Add Row below")
            duplicateAction = menu.addAction("Duplicate Row below")
            removeAction = menu.addAction("Delete Row")
            action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
            if action == addAction:
                self.tableModel.insertRows(idx.row()+1, 1, QModelIndex())
            elif action == duplicateAction:
                self.tableModel.duplicateRow(idx.row())
            elif action == removeAction:
                self.tableModel.removeRows(idx.row(), 1, QModelIndex())            
        else:
            #try if we get are at a row nevertheless
            idx = self.widget.indexAt(pos)
            if idx.isValid():
                addAction = menu.addAction("Add Row below")
                duplicateAction = menu.addAction("Duplicate Row below")
                action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
                if action == addAction:
                    self.tableModel.insertRows(idx.row()+1, 1, QModelIndex())
                elif action == duplicateAction:
                    self.tableModel.duplicateRow(idx.row())
            
            else:
                addAction = menu.addAction("Add Row to end")
                action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
                if action == addAction:
                    self.tableModel.insertRows(self.tableModel.rowCount(None), 1, QModelIndex())
            
        
            
            
    def cellPopUp(self, pos):
        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i
        
        
        
        menu = QMenu()
        
        if idx is None or not idx.isValid():
            addAction = menu.addAction("Add Row to end")
            action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
            if action == addAction:
                self.tableModel.insertRows(self.tableModel.rowCount(None), 1, QModelIndex())
            return
        
        #action to pop up from property field
        setFromPropertyAction = None
        if self.role == Qt.EditRole:
            setFromPropertyAction = menu.addAction("From device property")
        
        
        #check if this cell can be set to a default value
        col = idx.column()
    
        setDefaultAction = None
       
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
        else:
            self.role = Qt.EditRole
        self.tableModel.setRole(self.role)
        self._setComboBoxes(ro)
        
        
        
            