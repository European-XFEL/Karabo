#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QCursor, QTabWidget

from .placeholderpanel import PlaceholderPanel


class PanelContainer(QTabWidget):
    """A container for ``BasePanelWidget`` instances
    """
    def __init__(self, title, parent, allow_closing=False, handle_empty=False):
        super(PanelContainer, self).__init__(parent)
        self.setWindowTitle(title)
        self.panel_set = set()
        self.allow_closing = allow_closing
        self.handle_empty = handle_empty
        self.tabCloseRequested.connect(self.onCloseTab)

        if self.handle_empty:
            self._add_placeholder()

    # ----------------------------------------------------------------------
    # public methods

    def addPanel(self, panel):
        """This function gets a panel and adds it to this container.
        """
        # Remove the placeholder if needed
        if self.handle_empty and self.count() == 1:
            first_panel = self.widget(0)
            if isinstance(first_panel, PlaceholderPanel):
                self.removeTab(0)

        index = self.addTab(panel, panel.windowTitle())
        panel.index = index

        # XXX: Circular references hurrah!
        panel.attach_to_container(self)

        # Store panel in set to keep it alive for un/dock event!!!
        self.panel_set.add(panel)
        self._update_tabs_closable()

    def removePanel(self, panel):
        # trigger this to class closeEvent for panels and destroy connections
        # properly
        panel.force_close()
        # Then go about the removal of the tab
        index = self.indexOf(panel)
        if index < 0:
            # dock again to get rid of it
            panel.onDock()
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)
        self._update_tabs_closable()

        # Add a placeholder if needed
        if self.handle_empty and self.count() == 0:
            self._add_placeholder()

    def undock(self, panel):
        if panel.parent() is not None:
            self.removeTab(panel.index)
            panel.is_docked = False
            panel.setParent(None)
            panel.move(QCursor.pos())
            panel.show()

            if self.count() == 0:
                self.hide()

    def dock(self, panel):
        if panel.parent() is None:
            index = self.insertTab(panel.index, panel, panel.windowTitle())
            panel.is_docked = True

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).index = i

            self.setCurrentIndex(index)
            self.show()

    # --------------------------------------
    # Qt Overrides

    def addTab(self, widget, label):
        index = super(PanelContainer, self).addTab(widget, label)
        self._set_tab_text_color(index, widget)
        return index

    def insertTab(self, index, widget, label):
        index = super(PanelContainer, self).insertTab(index, widget, label)
        self._set_tab_text_color(index, widget)
        return index

    # ----------------------------------------------------------------------
    # slots

    @pyqtSlot(int)
    def onCloseTab(self, index):
        if self.count() == 1:
            return

        # Get panel, which is about to be closed
        panel = self.widget(index)
        # Close panel (if possible) before removing it from tab
        if not panel.force_close():
            return
        # Remove panel from tab
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)
        self._update_tabs_closable()

    # ----------------------------------------------------------------------
    # private methods

    def _add_placeholder(self):
        placeholder = PlaceholderPanel()
        self.addTab(placeholder, placeholder.windowTitle())

    def _set_tab_text_color(self, index, widget):
        color = widget.tab_text_color()
        if color is not None and color.isValid():
            tab_bar = self.tabBar()
            tab_bar.setTabTextColor(index, color)

    def _update_tabs_closable(self):
        if self.count() > 1 and not self.tabsClosable():
            self.setTabsClosable(self.allow_closing)
        elif self.count() == 1:
            self.setTabsClosable(False)
