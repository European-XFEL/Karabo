#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsView."""

__all__ = ["GraphicsView"]
    
from customxmlreader import CustomXmlReader
from customxmlwriter import CustomXmlWriter

from displaycomponent import DisplayComponent

from enums import NavigationItemTypes
from enums import ConfigChangeTypes
from enums import CompositionMode

from layoutcomponents.arrow import Arrow
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.linkbase import LinkBase
from layoutcomponents.nodebase import NodeBase
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text
from layoutcomponents.textdialog import TextDialog

from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree

ns_svg = "{http://www.w3.org/2000/svg}"
ns_karabo = "{http://karabo.eu/scene}"
ElementTree.register_namespace("svg", ns_svg[1:-1])
ElementTree.register_namespace("krb", ns_karabo[1:-1])

def _parse_rect(elem):
    if elem.get(ns_karabo + "x") is not None:
        ns = ns_karabo
    else:
        ns = ""
    return QRect(float(elem.get(ns + "x")), float(elem.get(ns + "y")),
                 float(elem.get(ns + "width")), float(elem.get(ns + "height")))

class Label(object):
    """ Fake class to make a QLabel loadable """
    @staticmethod
    def load(elem, parent):
        label = QLabel(elem.get(ns_karabo + "text"), parent)
        ret = ProxyWidget(parent)
        ret.set_child(label, None)
        ret.show()
        return ret

class Layout(object):
    def element(self):
        g = self.geometry()
        d = { ns_karabo + "x": g.x(), ns_karabo + "y": g.y(),
              ns_karabo + "width": g.width(), ns_karabo + "height": g.height(),
              ns_karabo + "class": self.__class__.__name__ }
        d.update(self.save())

        e = ElementTree.Element(ns_svg + "g",
                                {k: unicode(v) for k, v in d.iteritems()})
        e.extend(l.widget().element() if isinstance(l, QWidgetItem) else
                 l.element() for l in (self.itemAt(i)
                                       for i in range(self.count())))
        return e


class FixedLayout(QLayout, Layout):
    def __init__(self, parent):
        QLayout.__init__(self, parent)
        self.children = [ ]
        self.geometries = { }

    def addItem(self, item, geometry=None):
        if geometry is None:
            self._item = item
        if isinstance(item, QWidget):
            self.addWidget(item)
            item = self._item
        self.children.append(item)
        self.geometries[item] = geometry
        self.update()

    def itemAt(self, index):
        try:
            return self.children[index]
        except IndexError:
            return

    def takeAt(self, index):
        try:
            return self.children.pop(index)
        except IndexError:
            return
        
    def count(self):
        return len(self.children)

    def setGeometry(self, geometry):
        for item in self.children:
            item.setGeometry(self.geometries[item])

    def setItemPosition(self, item, pos):
        rect = QRect(pos, item.geometry().size())
        self.geometries[item] = rect
        item.setGeometry(rect)

    def sizeHint(self):
        return QSize(10, 10)

    def itemAtPosition(self, pos):
        for item in self.children:
            if item.geometry().contains(pos):
                return item
            
    def relayout(self, widget):
        for c in self.children:
            ll = [c]
            ret = None
            while ll:
                l = ll.pop()
                if isinstance(l, QLayout):
                    for i in range(l.count()):
                        ll.append(l.itemAt(i))
                else:
                    if l.widget() is widget:
                        ret = l
                        break
            if ret is not None:
                break
        if ret is None:
            return
        print c.sizeHint()
        g = QRect(self.geometries[c].topLeft(), c.sizeHint())
        self.geometries[c] = g
        c.setGeometry(g)

    def save(self):
        return { }

class BoxLayout(QBoxLayout, Layout):
    def __init__(self, dir):
        QBoxLayout.__init__(self, dir)
        self.setContentsMargins(5, 5, 5, 5)

    @staticmethod
    def load(elem, parent):
        ret = BoxLayout(int(elem.get(ns_karabo + "Direction")))
        for i in range(len(elem)):
            cls = elem[i].get(ns_karabo + "class")
            item = globals()[cls].load(elem[i], parent)
            if isinstance(item, QWidget):
                ret.addWidget(item)
            else:
                ret.addLayout(item)
        return ret

    def save(self):
        return {ns_karabo + "Direction": self.direction()}

class ProxyWidget(QStackedWidget):
    def __init__(self, parent):
        QStackedWidget.__init__(self, parent)
        #self.setContextMenuPolicy(Qt.ActionsContextMenu)
        self.change_widget = QAction("Change widget", self)
        self.addAction(self.change_widget)

    def set_child(self, child, component):
        if self.count() > 0:
            self.removeWidget(self.widget(0))
        self.addWidget(child)
        self.component = component
        if component is not None:
            aliases = component.widgetFactory.getAliasesViaCategory(
                            None, self.component.widgetCategory)
            menu = QMenu(self)
            for a in aliases:
                menu.addAction(a).triggered.connect(self.on_changeWidget)
            self.change_widget.setMenu(menu)

    @pyqtSlot()
    def on_changeWidget(self):
        action = self.sender()
        self.component.changeWidget(action.text())
        self.adjustSize()
        self.parent().layout().relayout(self)
        
    def contextMenuEvent(self, event):
        if not self.parent().parent().isDesignMode:
            return
        QMenu.exec_(self.actions(), event.globalPos(), None, self)

    def element(self):
        g = self.geometry()
        d = { "x": g.x(), "y": g.y(), "width": g.width(), "height": g.height() }
        if self.component is None:
            d[ns_karabo + "class"] = "Label"
            d[ns_karabo + "text"] = self.widget(0).text()
        else:
            widgetFactory = self.component.widgetFactory
            d[ns_karabo + "class"] = "ProxyWidget"
            d[ns_karabo + "componentType"] = self.component.__class__.__name__
            d[ns_karabo + "widgetFactory"] = "DisplayWidget"
            d[ns_karabo + "classAlias"] = self.component.classAlias
            if self.component.classAlias == "Command":
                d[ns_karabo + "commandText"] = widgetFactory.widget.text()
                d[ns_karabo + "commandEnabled"] = "{}".format(
                    widgetFactory.widget.isEnabled())
                d[ns_karabo + "allowedStates"] = ",".join(
                    widgetFactory.allowedStates)
                d[ns_karabo + "command"] = widgetFactory.command
            d[ns_karabo + "key"] = ",".join(self.component.keys)
        return ElementTree.Element(ns_svg + "rect",
                                   {k: unicode(v) for k, v in d.iteritems()})

    @staticmethod
    def load(elem, parent):
        ks = "classAlias", "key", "widgetFactory"
        if elem.get(ns_karabo + "classAlias") == "Command":
            ks += "command", "allowedStates", "commandText"
        d = {k: elem.get(ns_karabo + k) for k in ks}
        d["commandEnabled"] = elem.get(ns_karabo + "commandEnabled") == "True"
        component = globals()[elem.get(ns_karabo + "componentType")](**d)
        component.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        ret = ProxyWidget(parent)
        ret.set_child(component.widget, component)
        ret.show()
        return ret

            
class GraphicsView(QSvgWidget):
    # Enums
    MoveItem, InsertText, InsertLine, InsertRect = range(4)
    # Signals
    lineInserted = pyqtSignal()
    rectInserted = pyqtSignal()
    sceneSelectionChanged = pyqtSignal()

    def __init__(self):
        super(GraphicsView, self).__init__()
        
        self.inner = QWidget(self)
        self.layout = FixedLayout(self.inner)
        self.inner.setLayout(self.layout)
        layout = QStackedLayout(self)
        layout.addWidget(self.inner)
        
        self.moving_item = None
        self.selection = [ ]
        self.selection_start = None

        self.tree = ElementTree.ElementTree(ElementTree.Element(ns_svg + "svg"))

        # Current mode of the view (move, insert)
        self.__mode = self.MoveItem
        
        # Composition mode is either ON/OFFLINE, once set not changeable
        self.__compositionMode = CompositionMode.UNDEFINED

        self.__line = None
        self.__rect = None

        self.setDesignMode(True)

        self.__minZ = 0
        self.__maxZ = 0
        self.__seqNumber = 0

        self.__rotAngle = 30

        # Describes most recent item to be cut or copied inside the application
        self.__copiedItem = QByteArray()

        self.setAcceptDrops(True)
        

    def _getMode(self):
        return self.__mode
    def _setMode(self, mode):
        self.__mode = mode
    mode = property(fget=_getMode, fset=_setMode)


    def _getDesignMode(self):
        return self.__isDesignMode
    isDesignMode = property(fget=_getDesignMode)


    # Sets all items in design or control mode
    def setDesignMode(self, isDesignMode):
        self.__isDesignMode = isDesignMode
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, 
                                      isDesignMode)


    # Returns true, when items has been copied; otherwise false
    def hasCopy(self):
        return (len(self.__copiedItem) > 0)


    # All selected items of the scene are returned
    def selectedItems(self):
        return [ ]


    # If there are exactely 2 selected items (not of type Link) they are returned
    # as a pair; otherwise None is returned
    def selectedItemPair(self):
        items = self.selectedItems()
        if len(items) == 2:
            firstItem = items[0]
            secondItem = items[1]

            if isinstance(firstItem, Link):
                firstItem = None
            if isinstance(secondItem, Link):
                secondItem = None

            if firstItem and secondItem:
                return (firstItem, secondItem)

        return None


    # All selected items of type Text are returned
    def selectedTextItems(self):
        items = self.selectedItems()

        if len(items) > 0:
            textItems = []
            for item in items:
                if isinstance(item, Text):
                    textItems.append(item)
            return textItems
        return None


    # All selected items of type Link are returned
    def selectedLinks(self):
        items = self.selectedItems()

        if len(items) > 0:
            linkItems = []
            for item in items:
                if isinstance(item, LinkBase):
                    linkItems.append(item)
            if len(linkItems) > 0:
                return linkItems
        return None


    def selectedItemGroup(self):
        items = self.selectedItems()

        if len(items) > 0:
            groupItems = []
            for item in items:
                if isinstance(item, QGraphicsItemGroup):
                    groupItems.append(item)
            if len(groupItems) > 0:
                return groupItems
        return None


    # Open saved view from file
    def openSceneLayoutFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open saved view",
                                               filter="SVG (*.svg)")
        if len(filename) < 1:
            return

        self.openScene(filename)


    def openSceneConfigurationsFromFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to open configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        dir = QDir(dirPath)
        fileInfos = dir.entryInfoList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)
        for fileInfo in fileInfos:
            print fileInfo


    def openSceneLayoutConfigurationsFromFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to open configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        dir = QDir(dirPath)
        fileInfos = dir.entryInfoList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)

        internalKeyTextTuples = []
        for fileInfo in fileInfos:
            if fileInfo.suffix() == "scene":
                # Layout file
                internalKeyTextTuples = self.openScene(str(fileInfo.absoluteFilePath()))
            #elif fileInfo.suffix() == "xml":
                # Configuration file
                #print "XML file:", fileInfo.absoluteFilePath()

        for internalKeyText in internalKeyTextTuples:
            internalKey = str(internalKeyText[0])
            text = str(internalKeyText[1])
            filename = str(dirPath + "/" + text + ".xml")
            # openAsXml(self, filename, internalKey, configChange=ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED, classId=str())

            # TODO: Remove dirty hack for scientific computing again!!!
            croppedClassId = text.split("-")
            classId = croppedClassId[0]
            Manager().openAsXml(filename, internalKey, ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED, classId)


    # Helper function opens *.scene file
    # Returns list of tuples containing (internalKey, text) of GraphicsItem of scene
    def openScene(self, filename):
        self.tree = ElementTree.parse(filename)
        root = self.tree.getroot()
        i = 0
        while i < len(root):
            elem = root[i]
            cls = elem.get(ns_karabo + "class")
            if cls is None:
                i += 1
            else:
                obj = globals()[cls].load(elem, self.inner)
                self.layout.addItem(obj, _parse_rect(elem))
                del root[i]

        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        self.tree.write(buf)
        buf.close()
        self.load(ar)


    # Helper function opens *.xml configuration file
    def openConfiguration(self, filename):
        print "openConfiguration", filename


    # Save active view to file
    def saveSceneLayoutToFile(self):
        filename = QFileDialog.getSaveFileName(None, "Save file as",
                                               filter="SVG (*.svg)")
        if len(filename) < 1:
            return

        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".svg"

        root = self.tree.getroot().copy()
        tree = ElementTree.ElementTree(root)
        e = self.layout.element()
        root.extend(ee for ee in e)
        tree.write(filename)


    def saveSceneConfigurationsToFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to save configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        # Check, if directory is empty
        self.checkDirectoryBeforeSave(dirPath)

        # Save configurations of navigation related items
        self.saveSceneConfigurations(dirPath)


    # Save active view and configurations to folder/files
    def saveSceneLayoutConfigurationsToFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to save layout and configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        # Check, if directory is empty
        self.checkDirectoryBeforeSave(dirPath)

        # Save layout to directory
        CustomXmlWriter(self.__scene).write(dirPath + "/" + QDir(dirPath).dirName() + ".scene")

        # Save configurations of navigation related items
        self.saveSceneConfigurations(dirPath)


    # Helper function checks whether the directory to save to is empty
    # If the directory is not empty the user has to select what happens with the existing files
    def checkDirectoryBeforeSave(self, dirPath):
        dir = QDir(dirPath)
        files = dir.entryList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)
        if len(files) > 0:
            reply = QMessageBox.question(self, 'Selected directory is not empty',
                "The selected directory already contains files.<br>These files will be overwritten or removed.<br><br>" \
                + "Do you want to continue?", QMessageBox.Yes |
                QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        for file in files:
            dir.remove(file)


    # Helper function to save all configurations for scene items
    def saveSceneConfigurations(self, dirPath):
        items = self.items()
        for item in items:
            if isinstance(item, GraphicsCustomItem):
                # TODO: Remove dirty hack for scientific computing again!!!
                croppedClassId = item.text().split("-")
                classId = croppedClassId[0]
                Manager().saveAsXml(str(dirPath + "/" + item.text() + ".xml"), str(classId), str(item.internalKey()))


    # A new instance of a text is created and passed to the _setupItem function
    # to position and select
    def addText(self):
        textDialog = TextDialog(self)
        if textDialog.exec_() == QDialog.Rejected:
            return

        textItem = Text(self.__isDesignMode)
        textItem.setText(textDialog.text())
        textItem.setFont(textDialog.font())
        textItem.setTextColor(textDialog.textColor())
        textItem.setBackgroundColor(textDialog.backgroundColor())
        textItem.setOutlineColor(textDialog.outlineColor())

        self._setupItem(textItem)


    # Add a link, if exactely 2 items are selected
    def addLink(self):
        items = self.selectedItemPair()
        if items is None:
            return

        link = Link(items[0], items[1])
        self.__scene.addItem(link)


    # Add an arrow, if exactely 2 items are selected
    def addArrowLink(self):
        items = self.selectedItemPair()
        if items is None:
            return

        arrowLink = Arrow(items[0], items[1])
        self.__scene.addItem(arrowLink)


    # Cut is two-part process: copy selected items and remove item from scene
    def cut(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Copy items
        self.copy()
        # Remove items from scene
        for item in items:
            self.__scene.removeItem(item)
            del item


    # The selected items are stored as binary data inside the application
    def copy(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Copy data into DataStream
        self.__copiedItem.clear()
        stream = QDataStream(self.__copiedItem, QIODevice.WriteOnly)
        for item in items:
            if isinstance(item, Text):
                stream << "Text" << item.text() \
                                          << item.font().toString() \
                                          << item.textColor().name() \
                                          << item.outlineColor().name() \
                                          << item.backgroundColor().name() \
                                          << "\n"
            elif isinstance(item, Link):
                print "Link"
            elif isinstance(item, Arrow):
                print "Arrow"
            elif isinstance(item, Line):
                line = item.line()
                stream << "Line" << "{},{},{},{}".format(
                        line.x1(), line.y1(), line.x2(), line.y2()) \
                    << "{}".format(item.length()) \
                    << "{}".format(item.widthF()) \
                    << "{}".format(item.style()) \
                    << item.color().name() \
                    << "\n"
            elif isinstance(item, Rectangle):
                rect = item.rect()
                topLeft = rect.topLeft()
                stream << "Rectangle" << "{},{},{},{}".format(
                    topLeft.x(), topLeft.y(), rect.width(), rect.height()) \
                    << "\n"
            elif isinstance(item, GraphicsProxyWidgetContainer):
                print "GraphicsProxyWidgetContainer"
            elif isinstance(item, GraphicsProxyWidget):
                print "GraphicsProxyWidget"
                embeddedWidget = item.widget()



    # The copied item data is extracted and the items are instantiated with the
    # refered binary data
    def paste(self):
        if len(self.__copiedItem) < 1:
            return

        stream = QDataStream(self.__copiedItem, QIODevice.ReadOnly)

        itemData = [ ]
        while not stream.atEnd():
            input = stream.readString()

            if input == "\n":
                # Create item
                type = itemData[0]
                if type == "Text" and len(itemData) == 6:
                    textItem = Text(self.__isDesignMode)
                    textItem.setText(itemData[1])
                    font = QFont()
                    font.fromString(itemData[2])
                    textItem.setFont(font)
                    textItem.setTextColor(QColor(itemData[3]))
                    textItem.setOutlineColor(QColor(itemData[4]))
                    textItem.setBackgroundColor(QColor(itemData[5]))
                    self._setupItem(textItem)
                elif type == "Link":
                    print "Link"
                elif type == "Arrow":
                    print "Arrow"
                elif type == "Line" and len(itemData) == 6:
                    lineItem = Line(self.__isDesignMode)
                    # Get line coordinates
                    lineCoords = itemData[1].split(",")
                    if len(lineCoords) == 4:
                        line = QLineF(lineCoords[0].toFloat()[0], lineCoords[1].toFloat()[0], \
                                      lineCoords[2].toFloat()[0], lineCoords[3].toFloat()[0])
                        lineItem.setLine(line)
                    # Get line length
                    length = float(itemData[2])
                    lineItem.setLength(length[0])
                    # Get line width
                    widthF = float(itemData[3])
                    lineItem.setWidthF(widthF[0])
                    # Get line style
                    style = int(itemData[4])
                    lineItem.setStyle(style[0])
                    # Get line color
                    lineItem.setColor(QColor(itemData[5]))

                    self._setupItem(lineItem)
                    lineItem.setTransformOriginPoint(lineItem.boundingRect().center())
                elif type == "Rectangle" and len(itemData) == 2:
                    rectItem = Rectangle(self.__isDesignMode)
                    rectData = itemData[1].split(",")
                    if len(rectData) == 4:
                        rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                      rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                        rectItem.setRect(rect)
                    self._setupItem(rectItem)
                    rectItem.setTransformOriginPoint(rectItem.boundingRect().center())
                elif type == "GraphicsProxyWidgetContainer":
                    print "GraphicsProxyWidgetContainer"
                elif type == "GraphicsProxyWidget":
                    print "GraphicsProxyWidget"

                itemData = [ ]
                continue

            itemData.append(input)


    # All selected items are removed; when an item (not type Link) is removed its
    # destructor deletes any links that are associated with it
    # To avoid double-deleting links, the Link-items are removed before deleting the other items
    def remove(self):
        items = self.selectedItems()
        if (len(items) and QMessageBox.question(self, "Remove selected items",
                                                "Remove {0} item{1}?".format(len(items),
                                                "s" if len(items) != 1 else ""),
                                                QMessageBox.Yes|QMessageBox.No) ==
                                                QMessageBox.No):
            return

        self.removeItems(items)


    def removeItems(self, items):
        while items:
            item = items.pop()
            if isinstance(item, Text) or isinstance(item, Rectangle):
                for link in item.links():
                    if link in items:
                        # Remove item from list - prevent double deletion
                        items.remove(link)
            elif isinstance(item, Link) or isinstance(item, Arrow):
                print "Link or Arrow removed"
            elif isinstance(item, Line):
                print "Line removed"
            elif isinstance(item, GraphicsProxyWidgetContainer):
                layout = item.layout()
                while layout.count() > 0:
                    proxyItem = layout.itemAt(layout.count()-1)
                    if not proxyItem:
                        continue
                    
                    keys = proxyItem.keys
                    if keys:
                        for key in keys:
                            Manager().removeVisibleDevice(key)
                    # Destroy and unregister
                    proxyItem.destroy()
                    if proxyItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(proxyItem)
                    
                    self.__scene.removeItem(proxyItem)
                    layout.removeItem(proxyItem)
                    
                item.destroy()
            elif isinstance(item, GraphicsProxyWidget):
                keys = item.keys
                if keys:
                    for key in keys:
                        Manager().removeVisibleDevice(key)
                # Destroy and unregister
                item.destroy()
            elif isinstance(item, GraphicsCustomItem):
                for inputItem in item.inputChannelItems():
                    if inputItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(inputItem)
                    self.__scene.removeItem(inputItem)
                for outputItem in item.outputChannelItems():
                    if outputItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(outputItem)
                    self.__scene.removeItem(outputItem)
                Manager().removeVisibleDevice(item.internalKey())
                Manager().unregisterEditableComponent(item.deviceIdKey, item)

            self.__scene.removeItem(item)
            del item


    # Rotates all selected items around 30 degrees
    def rotate(self):
        for item in self.selectedItems():
            #item.rotate(30)
            item.setRotation(self.__rotAngle)
            self.__rotAngle += 30


    # Scales all selected items up (so far) : TODO use value
    def scaleSelectedItemsUp(self): # value
        for item in self.selectedItems():
            item.scale(1.5, 1.5)


    def scaleSelectedItemsDown(self): # value
        for item in self.selectedItems():
            item.scale(0.75, 0.75)


    def groupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Unselect all selected items
        for item in items:
            item.setSelected(False)

        itemGroup = self.__scene.createItemGroup(items)
        itemGroup.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
        itemGroup.setSelected(True)
        self.bringToFront()


    def horizontalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Horizontal, items)


    def verticalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Vertical, items)


    def breakLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        for item in items:
            self.__scene.breakLayout(item)


    def unGroupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        childItems = []
        for item in items:
            if isinstance(item, QGraphicsItemGroup):
                childItems = item.childItems()
                self.__scene.destroyItemGroup(item)
        # Select all items again
		for childItem in childItems:
		    childItem.setSelected(True)


    # Increments self.__maxZ value, and then sets the currently selected item's z
    # value to self.__maxZ
    def bringToFront(self):
        self.__maxZ += 1
        self._setZValue(self.__maxZ)


    # Decrements self.__minZ value, and then sets the currently selected item's z
    # value to self.__minZ
    def sendToBack(self):
        self.__minZ -=1
        self._setZValue(self.__minZ)


### private ###
    # Positions a newly added or pasted item in the scene
    # The sequence number ensures that new items are added in different positions
    # rather than on top of each other
    def _setupItem(self, item):
        item.setPos(QPointF(80 + (100 * (self.__seqNumber % 5)), 80 + (50 * ((self.__seqNumber / 5) % 7))))
        self.__seqNumber += 1
        self._addItem(item)


    def _addItem(self, item):
        self.__scene.addItem(item)
        self.__scene.clearSelection()
        item.setSelected(True)
        self.bringToFront()


    # Sets the z value of all selected items to z
    def _setZValue(self, z):
        items = self.selectedItems()
        for item in items:
            item.setZValue(z)


    # Creates and returns container item
    def createGraphicsItemContainer(self, orientation, items, pos):
        # Initialize layout
        layout = BoxLayout(QBoxLayout.LeftToRight if
                            orientation == Qt.Horizontal else 
                            QBoxLayout.TopToBottom)

        for item, component in items:
            proxy = ProxyWidget(self.inner)
            proxy.set_child(item, component)
            layout.addWidget(proxy)
            proxy.show()
            
        self.layout.addItem(layout, QRect(pos, layout.sizeHint()))
        return layout


### protected ###
    #def wheelEvent(self, event):
    #    #factor = 1.41 ** (-event.delta() / 240.0)
    #    factor = 1.0 + (0.2 * qAbs(event.delta()) / 120.0)
    #    if event.delta() > 0:
    #        self.scale(factor, factor)
    #    else:
    #        factor = 1.0/factor
    #        self.scale(factor, factor)
    #    QGraphicsView.wheelEvent(self, event)


    def mousePressEvent(self, event):
        if not self.isDesignMode:
            return
        if event.button() == Qt.LeftButton:
            item = self.layout.itemAtPosition(event.pos())
            if item is None:
                self.selection_stop = self.selection_start = event.pos()
                self.update()
            if item is not None:
                self.moving_item = item
                self.moving_pos = (
                    event.pos() - self.moving_item.geometry().topLeft())
                if event.modifiers() & Qt.ShiftModifier:
                    self.selection.append(item)
                else:
                    self.selection = [item]
                self.update()
                event.accept()
        else:
            child = self.inner.childAt(event.pos())
            if child is not None:
                while not isinstance(child, ProxyWidget):
                    child = child.parent()
                child.mousePressEvent(event)

        # Items are created in origin and must then be moved to the position to
        # set their position correctly for later purposes!!!
        if self.__mode == self.InsertLine:
            self.__line = Line(self.__isDesignMode)
            self._addItem(self.__line)
            self.__line.setPos(pos.x(), pos.y())
        elif self.__mode == self.InsertRect:
            self.__rect = Rectangle(self.__isDesignMode)
            self._addItem(self.__rect)
            self.__rect.setPos(pos.x(), pos.y())

        QWidget.mousePressEvent(self, event)

    def contextMenuEvent(self, event):
        if not self.isDesignMode:
            return
        child = self.inner.childAt(event.pos())
        if child is not None:
            while not isinstance(child, ProxyWidget):
                child = child.parent()
            child.event(event)

    def mouseMoveEvent(self, event):
        if self.moving_item is not None:
            self.layout.setItemPosition(self.moving_item, event.pos() - 
                                        self.moving_pos)
            event.accept()
        elif self.selection_start is not None:
            self.selection_stop = event.pos()
            event.accept()
            self.update()
    
        if self.__mode == self.InsertLine and self.__line:
            linePos = self.__line.pos()
            pos = QPointF(pos.x()-linePos.x(), pos.y()-linePos.y())
            newLine = QLineF(QPointF(), QPointF(pos))
            self.__line.setLine(newLine)
        elif self.__mode == self.InsertRect and self.__rect:
            rectPos = self.__rect.pos()
            pos = QPointF(pos.x()-rectPos.x(), pos.y()-rectPos.y())
            newRect = QRectF(QPointF(), QPointF(pos))
            self.__rect.setRect(newRect)
        #elif self.__mode == self.MoveItem:
        self.update()
        QWidget.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        self.moving_item = None
        if self.selection_start is not None:
            rect = QRect(self.selection_start, self.selection_stop)
            if event.modifiers() & Qt.ShiftModifier:
                sel = self.selection
            else:
                sel = [ ]
            for c in self.layout.children:
                if rect.contains(c.geometry()):
                    sel.append(c)
            self.selection = sel
            self.selection_start = None
            event.accept()
            self.update()

        if self.__line and self.__mode == self.InsertLine:
            centerPos = self.__line.boundingRect().center()
            self.__line.setTransformOriginPoint(centerPos)
            self.__line.setSelected(True)
            self.lineInserted.emit()
        elif self.__rect and self.__mode == self.InsertRect:
            rect = self.__rect.boundingRect()
            centerPos = rect.center()
            self.__rect.setTransformOriginPoint(centerPos)
            self.__rect.setSelected(True)
            self.rectInserted.emit()

        self.__line = None
        self.__rect = None
        QWidget.mouseReleaseEvent(self, event)


# Drag & Drop events
    def dragEnterEvent(self, event):
        #print "GraphicsView.dragEnterEvent"

        source = event.source()
        if (source is not None) and (source is not self):
            event.accept()

        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):
        #print "GraphicsView.dragMoveEvent"
        event.accept()
        #QGraphicsView.dragMoveEvent(self, event)


    def dropEvent(self, event):
        #print "GraphicsView.dropEvent"

        source = event.source()
        if source is not None:
            customItem = None
            mimeData = event.mimeData()
            # Source type
            sourceType = mimeData.data("sourceType")
            # Drop from NavigationTreeView or ParameterTreeWidget?
            if sourceType == "NavigationTreeView":
                # Navigation item type
                navItemType = int(mimeData.data("navigationItemType"))
                # Device server instance id
                serverId = mimeData.data("serverId").data()
                # Internal key
                #key = mimeData.data("key").data()
                # Display name
                displayName = mimeData.data("displayName").data()
                
                # Get schema
                schema = None
                path = str("server." + serverId + ".classes." + displayName + ".description")
                if Manager().hash.has(path):
                    schema = Manager().hash.get(path)
                
                configKey, configCount = Manager().createNewConfigKeyAndCount(displayName)
                
                # Create graphical item
                customItem = GraphicsCustomItem(configKey, self.__isDesignMode, displayName, schema, (navItemType == NavigationItemTypes.CLASS))
                tooltipText = "<html><b>Associated key: </b>%s</html>" % configKey
                customItem.setToolTip(tooltipText)
                offset = QPointF()
                # Add created item to scene
                self._addItem(customItem)

                # Register as visible device - TODO?
                #Manager().newVisibleDevice(configKey)

                if navItemType and (navItemType == NavigationItemTypes.CLASS):
                    Manager().createNewProjectConfig(customItem, configKey, configCount, displayName, schema)

                    # Connect customItem signal to Manager, DEVICE_CLASS
                    customItem.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
                    # Register for value changes of deviceId
                    Manager().registerEditableComponent(customItem.deviceIdKey, customItem)

            elif sourceType == "ParameterTreeWidget":                
                # Internal key
                internalKey = mimeData.data("internalKey").data()
                # Display name
                displayName = mimeData.data("displayName").data()
                # Display component?
                hasDisplayComponent = mimeData.data(
                    "hasDisplayComponent") == "True"
                # Editable component?
                hasEditableComponent = mimeData.data(
                    "hasEditableComponent") == "True"
                
                # TODO: HACK to get apply button disabled
                currentValue = None
                if hasEditableComponent:
                    currentValue = str(mimeData.data("currentValue"))
                
                metricPrefixSymbol = mimeData.data("metricPrefixSymbol").data()
                unitSymbol = mimeData.data("unitSymbol").data()
                
                enumeration = mimeData.data("enumeration").data()
                if enumeration:
                    enumeration = enumeration.split(",")
                # Navigation item type
                navItemType = int(mimeData.data("navigationItemType"))
                # Class alias
                classAlias = mimeData.data("classAlias").data()

                # List stored all items for layout
                items = []

                displayNameProxyWidget = self._createDisplayNameProxyWidget(displayName)
                if displayNameProxyWidget:
                    # Add item to itemlist
                    items.append((displayNameProxyWidget, None))

                # Does key concern state of device?
                keys = str(internalKey).split('.configuration.')
                isStateToDisplay = keys[1] == "state"

                # Display widget
                if hasDisplayComponent:
                    # Special treatment for command
                    if classAlias == "Command":
                        allowedStates = []
                        displayText = str()
                        commandEnabled = False
                        command = str()
                        parameterItem = source.getParameterTreeWidgetItemByKey(internalKey)
                        if parameterItem:
                            allowedStates = parameterItem.allowedStates
                            displayText = parameterItem.displayText
                            commandEnabled = parameterItem.enabled
                            command = parameterItem.command
                        displayComponent = DisplayComponent(classAlias, key=internalKey, \
                                                            allowedStates=allowedStates, \
                                                            commandText=displayText, \
                                                            commandEnabled=commandEnabled, \
                                                            command=command)
                    else:
                        displayComponent = DisplayComponent(classAlias, key=internalKey, \
                                                            enumeration = enumeration, \
                                                            metricPrefixSymbol=metricPrefixSymbol, \
                                                            unitSymbol=unitSymbol)
                    
                    items.append((displayComponent.widget, displayComponent))
                    
                    # Add proxyWidget for unit label, if available
                    unitProxyWidget = self._createUnitProxyWidget(metricPrefixSymbol, unitSymbol)
                    if unitProxyWidget:
                        items.append((unitProxyWidget, None))

                    # Register as visible device
                    Manager().newVisibleDevice(internalKey)

                # Editable widget
                if hasEditableComponent:
                    if navItemType is NavigationItemTypes.CLASS:
                        editableComponent = EditableNoApplyComponent(classAlias, key=internalKey, \
                                                                     enumeration = enumeration, \
                                                                     metricPrefixSymbol=metricPrefixSymbol, \
                                                                     unitSymbol=unitSymbol)
                    elif navItemType is NavigationItemTypes.DEVICE:
                        editableComponent = EditableApplyLaterComponent(classAlias, key=internalKey, \
                                                                        enumeration = enumeration, \
                                                                        metricPrefixSymbol=metricPrefixSymbol, \
                                                                        unitSymbol=unitSymbol)
                        editableComponent.isEditableValueInit = False

                    items.append((editableComponent.widget, editableComponent))

                    # Register as visible device
                    Manager().newVisibleDevice(internalKey)                   

                customItem = self.createGraphicsItemContainer(
                    Qt.Horizontal, items, event.pos())

            if customItem is None:
                return

        event.accept()

        QWidget.dropEvent(self, event)


    def _getWidgetCenterPosition(self, pos, centerX, centerY):
        # QPointF pos, int centerX, int centerY
        pos.setX(pos.x()-centerX)
        pos.setY(pos.y()-centerY)
        return pos


    def _createDisplayNameProxyWidget(self, displayName):
        if len(displayName) < 1:
            return None

        # Label only, if there is something to show
        return QLabel(displayName)


    def _createUnitProxyWidget(self, metricPrefixSymbol, unitSymbol):
        # If metricPrefix &| unitSymbo are set a QLabel is returned,
        # otherwise None
        unitLabel = str()
        
        if len(metricPrefixSymbol) > 0:
            unitLabel += metricPrefixSymbol
        if len(unitSymbol) > 0:
            unitLabel += unitSymbol
        
        if len(unitLabel) > 0:
            laUnit = QLabel(unitLabel)
            return laUnit
        
        return None


### slots ###
    def onRemoveUserCustomFrame(self, userCustomFrame):
        print "onRemoveUserCustomFrame", userCustomFrame
        if userCustomFrame is None:
            return
        userCustomFrame.deleteLater()


    # Called whenever an item of the scene is un-/selected
    def onSceneSelectionChanged(self):
        self.sceneSelectionChanged.emit()

        if (len(self.__scene.selectedItems()) == 1):
            selectedItem = self.__scene.selectedItems()[0]
            if isinstance(selectedItem, GraphicsCustomItem):
                Manager().selectNavigationItemByKey(selectedItem.internalKey())
            elif isinstance(selectedItem, GraphicsProxyWidget):
                keys = selectedItem.keys
                if keys:
                    for key in keys:
                        Manager().selectNavigationItemByKey(key)
            elif isinstance(selectedItem, GraphicsProxyWidgetContainer):
                layout = selectedItem.layout()
                for i in xrange(layout.count()):
                    proxyItem = layout.itemAt(i)
                    keys = proxyItem.keys
                    if keys:
                        for key in keys:
                            Manager().selectNavigationItemByKey(key)

    def paintEvent(self, event):
        painter = QPainter(self)
        try:
            #self.renderer().render(painter, QRectF(QRect(QPoint(0, 0), self.renderer().defaultSize())))
            self.renderer().render(painter, self.renderer().viewBoxF())
            if self.isDesignMode:
                for item in self.layout.children:
                    if item in self.selection:
                        painter.setPen(Qt.green)
                    else:
                        painter.setPen(Qt.black)
                    painter.drawRect(item.geometry())
            if self.selection_start is not None:
                painter.setPen(Qt.black)
                painter.drawRect(QRect(self.selection_start, self.selection_stop))
        finally:
            painter.end()
