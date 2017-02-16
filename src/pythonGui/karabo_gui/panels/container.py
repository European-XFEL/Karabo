#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QCursor, QTabWidget


class PanelContainer(QTabWidget):
    """A container for ``BasePanelWidget`` instances
    """
    def __init__(self, title, parent):
        super(PanelContainer, self).__init__(parent)
        self.setWindowTitle(title)
        self.panel_set = set()
        self.tabCloseRequested.connect(self.onCloseTab)

    def addPanel(self, factory, label):
        """This function gets a panel factory, a label and optionally an icon,
        adds a new panel to the tab and returns it.
        """
        panel = factory(self, label)
        index = self.addTab(panel, label)
        panel.index = index

        # Store panel in set to keep it alive for un/dock event!!!
        self.panel_set.add(panel)
        return panel

    def removePanel(self, panel):
        index = self.indexOf(panel)
        if index < 0:
            # dock again to get rid of it
            panel.onDock()
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)
        self.updateTabsClosable()
        # trigger this to class closeEvent for panels and destroy connections
        # properly
        panel.close()

    def updateTabsClosable(self):
        if self.count() > 1 and not self.tabsClosable():
            self.setTabsClosable(True)
        elif self.count() == 1:
            self.setTabsClosable(False)

    def undock(self, panel):
        if panel.parent() is not None:
            self.removeTab(panel.index)
            panel.setParent(None)
            panel.move(QCursor.pos())
            panel.show()

            if self.count() == 0:
                self.hide()

    def dock(self, panel):
        if panel.parent() is None:
            index = self.insertTab(panel.index, panel, panel.title)

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).index = i

            self.setCurrentIndex(index)
            self.show()

    # ----------------------------------------------------------------------
    # slots

    @pyqtSlot(int)
    def onCloseTab(self, index):
        if self.count() == 1:
            return

        # Get panel, which is about to be closed
        panel = self.widget(index)
        # Close panel (if possible) before removing it from tab
        if not panel.forceClose():
            return
        # Remove panel from tab
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)
        self.updateTabsClosable()
