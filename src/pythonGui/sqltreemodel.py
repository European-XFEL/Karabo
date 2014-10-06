#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the model for a QTreeView based
   on a SQLite database.
"""

__all__ = ["SqlTreeModel"]

from sqltreemodellevel import SqlTreeModelLevel
from sqltreemodelnode import SqlTreeModelNode
from sqltreemodelprivate import SqlTreeModelPrivate

from PyQt4.QtCore import *


class SqlTreeModel(QAbstractItemModel):
    
    def __init__(self, parent=None):
        super(SqlTreeModel, self).__init__(parent)
        
        self.__privateModel = SqlTreeModelPrivate(self)


### public ###
    def appendModel(self, model):
        """Append the SQL model as the next level of the tree.
           @param model The SQL model to append."""
        self.__privateModel.appendLevelData(SqlTreeModelLevel(model))
        self.__privateModel.columns = -1


    def modelAt(self, level):
        """Return the SQL model at the given level.
           @param level The level of the tree (numbered from zero).
           @return The SQL model."""
        return self.__privateModel.levelDataAt(level).model


    def setColumnMapping(self, level, columnMapping):
        """Map tree model columns to SQL model columns.
           By default all columns are mapped except the primary and foreign keys.
           @param level The level of the tree (numbered from zero).
           @param columnMapping A list of column indices from the SQL model.
           Use -1 to indicate that a column is calculated."""
        
        levelData = self.__privateModel.levelDataAt(level)
        if levelData.columnMapping != columnMapping:
            levelData.columnMapping = columnMapping
            self.__privateModel.columns = -1;


    def updateData(self):
        """Rebuild the tree model after updating SQL models."""
        columnsChanged = False

        if self.__privateModel.columns == -1:
            for level in range(self.__privateModel.nbLevelData()):
                levelData = self.__privateModel.levelDataAt(level)

                if levelData.nbColumnMapping() == 0:
                    count = levelData.model.columnCount()
                    
                    if level == 0:
                        startIndex = 1
                    else:
                        startIndex = 2
                    for i in range(startIndex, count):
                        levelData.appendColumnMapping(i)

                # Always show just one column but have more columns in query
                self.__privateModel.columns = 1 #max(self.__privateModel.columns, levelData.nbColumnMapping())

            columnsChanged = True

        oldIndices = [] #QModelIndexList()
        oldLevels = [] #QList<int>
        oldIds = [] #QList<int>

        if not columnsChanged:
            self.layoutAboutToBeChanged.emit()

            oldIndices = self.persistentIndexList()

            for oldIndex in oldIndices:
                oldLevels.append(self.levelOf(oldIndex))
                oldIds.append(self.rowId(oldIndex))

        for levelData in self.__privateModel.levelData:
            levelData.clear()
        self.__privateModel.root.clearRows()

        for level in range(self.__privateModel.nbLevelData()):
            levelData = self.__privateModel.levelDataAt(level)
            model = levelData.model

            while model.canFetchMore():
                model.fetchMore()

            count = model.rowCount()

            levelData.resizeIds(count)
            if level > 0:
                levelData.resizeParentIds(count)

            for row in range(count):
                index = model.index(row, 0)
                id = int(model.data(index))
                levelData.setIdAt(row, id)

                if level == 0:
                    node = self.__privateModel.root
                else:
                    parentIndex = model.index(row, 1)
                    parentId = int(model.data(parentIndex))
                    levelData.setParentIdAt(row, parentId)

                    parentLevelData = self.__privateModel.levelDataAt(level-1)

                    parentRow = parentLevelData.idIndexOf(parentId)
                    if parentRow < 0:
                        continue

                    node = parentLevelData.getNodeValueByKey(parentRow)

                    if node is None:
                        node = SqlTreeModelNode(level, parentRow)
                        parentLevelData.insertNode(parentRow, node)

                node.appendRow(row)

        if not columnsChanged:
            newIndices = [] # QModelIndexList
            for i in range(len(oldIndices)):
                newIndices.append(self.findIndex(oldLevels[i], oldIds[i], oldIndices[i].column()))

            self.changePersistentIndexList(oldIndices, newIndices)
            self.layoutChanged.emit()
        else:
            self.reset()


    def levelOf(self, index):
        """Return the level of the given item.
           @param index The index of the item.
           @return The level of the item (numbered from zero) or -1 if index is
           not valid."""
        if not index.isValid():
            return -1

        node = index.internalPointer()
        if node is None:
            return -1;

        return node.level


    def mappedRow(self, index):
        """Return the row number in the SQL model of the given item.
           @param index The index of the item.
           @return The mapped row number or -1 if index is not valid."""
        
        if not index.isValid():
            return -1

        node = index.internalPointer()
        if node is None:
            return -1

        return node.rowValue(index.row(), -1)


    def mappedColumn(self, index):
        """Return the column number in the SQL model of the given item.
           @param index The index of the item.
           @return The mapped column number or -1 if index is not valid."""

        level = self.levelOf(index)
        if level < 0:
            return -1

        levelData = self.__privateModel.levelDataAt(level)

        return levelData.columnMappingValue(index.column(), -1)


    def rowId(self, index):
        """Return the primary key of the given item.
           @param index The index of the item.
           @return The value of the primary key or -1 if index is not valid."""

        level = self.levelOf(index)
        if level < 0:
            return -1

        levelData = self.__privateModel.levelDataAt(level)

        row = self.mappedRow(index)
        if row < 0:
            return -1

        return levelData.idAt(row)


    def rowParentId(self, index):
        """Return the foreign key of the given item.
           @param index The index of the item.
           @return The value of the foreign key or -1 if index is not valid."""

        level = self.levelOf(index)
        if level < 1:
            return -1

        levelData = self.__privateModel.levelDataAt(level)

        row = self.mappedRow(index)
        if row < 0:
            return -1

        return levelData.parentIdAt(row)


    def rawData(self, level, row, column, role=Qt.DisplayRole):
        """Return the data from the SQL model.
           @param level The level of the item (numbered from zero).
           @param row The mapped row number of the item.
           @param column The mapped column number of the item.
           @param role The role to retrieve.
           @return The data for the given cell and role."""

        if level < 0 or row < 0 or column < 0:
            return

        levelData = self.__privateModel.levelDataAt(level)
        model = levelData.model # QSqlQueryModel

        index = model.index(row, column)
        return model.data(index, role)


    def findIndex(self, level, id, column):
        """Return the index of an item with given level and primary key.
           @param level The level of the item (numbered from zero).
           @param id The primary key of the item.
           @param column The column of the cell to return.
           @return The item index or an invalid index if the item was not found."""

        if level < 0 or id < 0 or column < 0:
            return QModelIndex()

        levelData = self.__privateModel.levelDataAt(level)
        
        row = levelData.idIndexOf(id)
        if row < 0:
            return QModelIndex()

        if level == 0:
            node = self.__privateModel.root
        else:
            parentId = levelData.parentIdAt(row)

            parentLevelData = self.__privateModel.levelDataAt(level-1)

            parentRow = parentLevelData.idIndexOf(parentId)
            if parentRow < 0:
                return QModelIndex()

            node = parentLevelData.getNodeValueByKey(parentRow)
            if node is None:
                return QModelIndex()

        nodeRow = node.rowIndexOf(row)
        if nodeRow < 0:
            return QModelIndex()

        return self.createIndex(nodeRow, column, node)


    def findIndexByLevelRowCol(self, level, row, column):
        """Return the index of an item with given level and primary key.
           @param level The level of the item (numbered from zero).
           @param row The row of the cell to return.
           @param column The column of the cell to return.
           @return The item index or an invalid index if the item was not found."""

        if level < 0 or row < 0 or column < 0:
            return QModelIndex()

        levelData = self.__privateModel.levelDataAt(level)

        if level == 0:
            node = self.__privateModel.root
        else:
            parentId = levelData.parentIdAt(row)

            parentLevelData = self.__privateModel.levelDataAt(level-1)

            parentRow = parentLevelData.idIndexOf(parentId)
            if parentRow < 0:
                return QModelIndex()

            node = parentLevelData.getNodeValueByKey(parentRow)
            if node is None:
                return QModelIndex()

        nodeRow = node.rowIndexOf(row)
        if nodeRow < 0:
            return QModelIndex()

        return self.createIndex(nodeRow, column, node)


    def setSort(self, column, order=Qt.AscendingOrder):
        """Set the sort order without updating the model.
           @param column The index of the sort column.
           @param order The sort order."""
        self.__privateModel.sortColumn = column
        self.__privateModel.sortOrder = order


    def sortColumn(self):
        """Return the index of the current sort column."""
        return self.__privateModel.sortColumn


    def sortOrder(self):
        """Return the current sort order."""
        return self.__privateModel.sortOrder


### protected ###
    def updateQueries(self):
        """Called to update the queries of child SQL models after changing sort order."""
        pass


### Overrides ###
    def columnCount(self, parent=QModelIndex()):
        return max(self.__privateModel.columns, 0)


    def rowCount(self, parent=QModelIndex()):
        # Based on the internal data structures
        node = self.__privateModel.findNode(parent)

        if node is None:
            return 0
        return node.nbRows()


    def index(self, row, column, parent=QModelIndex()):
        # Based on the internal data structures
        node = self.__privateModel.findNode(parent)
        if node is None or row < 0 or row >= node.nbRows():
            return QModelIndex()
        return self.createIndex(row, column, node)


    def parent(self, index):
        # Based on the internal data structures
        if not index.isValid():
            return QModelIndex()

        node = index.internalPointer()
        if node is None:
            return QModelIndex()

        level = node.level
        if level < 1:
            return QModelIndex()

        row = node.index

        if level == 1:
            parentNode = self.__privateModel.root
        else:
            levelData = self.__privateModel.levelDataAt(level-1)
            parentId = levelData.parentIdAt(row)

            parentLevelData = self.__privateModel.levelDataAt(level-2)

            parentRow = parentLevelData.idIndexOf(parentId)
            if parentRow < 0:
                return QModelIndex()

            parentNode = parentLevelData.getNodeValueByKey(parentRow)
            if parentNode is None:
                return QModelIndex()

        nodeRow = parentNode.rowIndexOf(row)
        if nodeRow < 0:
            return QModelIndex()

        return self.createIndex(nodeRow, 0, parentNode)#(void*)parentNode)


    def data(self, index, role=Qt.DisplayRole):
        # Determines the level of the item and delegates the implementation to the model associated with the level
        # Maps the row and column from the tree model to the underlying SQL query model
        return self.rawData(self.levelOf(index), self.mappedRow(index), self.mappedColumn(index), role)


    def setHeaderData(self, section, orientation, value, role):
        if orientation != Qt.Horizontal or section < 0:
            return False

        if self.__privateModel.nbHeaders() <= section:
            self.__privateModel.resizeHeaders(max(section, 16))

        self.__privateModel.setHeadersAt(section, role, value)

        self.headerDataChanged.emit(orientation, section, section)

        return True


    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if orientation == Qt.Horizontal:
            value = self.__privateModel.getHeaderValueAt(section, role)

            if role == Qt.DisplayRole and not value.isValid():
                value = self.__privateModel.getHeaderValueAt(section, Qt.EditRole)

            if value.isValid():
                return value

            levelData = self.__privateModel.levelData[0]
            column = levelData.columnMappingValue(section, -1)
            if column >= 0:
                return levelData.model.headerData(column, Qt.Horizontal, role)

        return QAbstractItemModel.headerData(section, orientation, role)


    def sort(self, column, order=Qt.AscendingOrder):
        if self.rowCount() < 1:
            # Nothing to sort
            return
        
        if self.__privateModel.sortColumn != column or self.__privateModel.sortOrder != order:
            self.setSort(column, order)
            self.updateQueries()

