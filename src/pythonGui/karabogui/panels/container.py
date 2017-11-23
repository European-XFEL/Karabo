#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QCursor, QTabBar, QTabWidget

from .placeholderpanel import PlaceholderPanel


class PanelContainer(QTabWidget):
    """A container for ``BasePanelWidget`` instances
    """
    def __init__(self, title, parent, handle_empty=False):
        super(PanelContainer, self).__init__(parent)
        self.setWindowTitle(title)
        self.panel_set = set()
        self.handle_empty = handle_empty
        self.minimized = False
        self.maximized = False
        self.tabCloseRequested.connect(self.onCloseTab)

        self._add_placeholder()
        self._update_tabs_closable()

    # ----------------------------------------------------------------------
    # public methods

    def addPanel(self, panel):
        """This function gets a panel and adds it to this container.
        """
        self._remove_placeholder()

        # XXX: Circular references hurrah!
        panel.attach_to_container(self)

        # if the container is currently maximized, quitely add the new
        # tab in the background
        if not self.maximized:
            index = self.addTab(panel, panel.windowTitle())
            panel.index = index

        # The container might have been hidden
        # XXX: currently no way to know if other container is maximized,
        # so that we can decide if this container should stay hidden.
        if self.count() == 1:
            self.show()

        # Store panel in set to keep it alive for un/dock event!!!
        self.panel_set.add(panel)
        self._update_tabs_closable()

    def removePanel(self, panel):
        # trigger this to class closeEvent for panels and destroy connections
        # properly
        panel.close()
        # Then go about the removal of the tab
        index = self.indexOf(panel)
        if index < 0:
            # dock again to get rid of it
            panel.onDock()
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)
        if self.maximized:
            # restore other tabs before remove it
            panel.onMinimize()
        self._update_tabs_closable()

        self._add_placeholder()
        if self.count() == 0:
            self.hide()

    def minimize(self, minimized):
        """Minimize/unminimize a tab container.
        """
        self.minimized = minimized
        if minimized:
            self.hide()
        elif self.count() > 0:
            # Only show if the container is NOT empty!
            self.show()

    def dock(self, panel):
        if panel.parent() is None:
            self._remove_placeholder()

            index = self.insertTab(panel.index, panel, panel.windowTitle())
            panel.is_docked = True

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).index = i

            if self.maximized:
                # newly docked tab shouldn't appear and steal the focus if
                # the container is maximized with other panels
                self.removeTab(index)
            else:
                self.setCurrentIndex(index)
            self._update_tabs_closable()

            if not self.minimized:
                self.show()

    def undock(self, panel):
        if panel.parent() is not None:
            panel.is_docked = False
            panel.setParent(None)
            panel.move(QCursor.pos())
            panel.show()

            self._add_placeholder()
            if self.count() == 0:
                self.hide()

    def insert_panels_after_maximize(self, current_index):
        """When maximized, all panels are removed except for the selected one.
        This undoes that when the panel/container is minimized.
        """
        self.removeTab(0)
        # Add the tabs back to the container in sorted order
        panels = sorted(self.panel_set, key=lambda x: x.index)
        for pan in panels:
            if not pan.is_docked:
                continue
            self.insertTab(pan.index, pan, pan.windowTitle())

        self.setCurrentIndex(current_index)
        self._hide_close_buttons()

    def update_tab_text_color(self, panel, color):
        if panel.is_docked:
            index = self.indexOf(panel)
            self._set_tab_text_color(index, panel, color)

    # --------------------------------------
    # Qt Overrides

    def addTab(self, widget, label):
        index = super(PanelContainer, self).addTab(widget, label)
        self._set_tab_text_color(index, widget, widget.tab_text_color)
        return index

    def insertTab(self, index, widget, label):
        index = super(PanelContainer, self).insertTab(index, widget, label)
        self._set_tab_text_color(index, widget, widget.tab_text_color)
        return index

    # ----------------------------------------------------------------------
    # slots

    @pyqtSlot(int)
    def onCloseTab(self, index):
        # Get panel, which is about to be closed
        panel = self.widget(index)
        # Close panel (if possible) before removing it from tab
        if not panel.close():
            return
        # Remove panel from tab
        self.removeTab(index)
        panel.setParent(None)
        self.panel_set.remove(panel)

        if self.maximized:
            panel.onMinimize()

        self._add_placeholder()
        # if all tabs are closed, hide myself
        if self.count() == 0:
            self.hide()

    # ----------------------------------------------------------------------
    # private methods

    def _add_placeholder(self):
        """Add a placeholder if needed
        """
        if self.handle_empty and self.count() == 0:
            placeholder = PlaceholderPanel()
            self.addTab(placeholder, placeholder.windowTitle())
            self._update_tabs_closable()

    def _remove_placeholder(self):
        """Remove the placeholder if needed
        """
        if self.handle_empty and self.count() == 1:
            if isinstance(self.widget(0), PlaceholderPanel):
                self.removeTab(0)

    def _set_tab_text_color(self, index, widget, color=None):
        if color is not None and color.isValid():
            tab_bar = self.tabBar()
            tab_bar.setTabTextColor(index, color)

    def _update_tabs_closable(self):
        if self.count() > 1:
            self.setTabsClosable(True)
        elif self.count() == 1:
            is_placeholder = isinstance(self.widget(0), PlaceholderPanel)
            self.setTabsClosable(not is_placeholder)
        self._hide_close_buttons()

    def _hide_close_buttons(self):
        """Remove the close button non-closable panels
        """
        # use index instead of loop through set, because need the index
        # for tabBar button anyway.
        i = self.count()
        while i > -1:
            i -= 1
            widget = self.widget(i)
            if widget is not None and not widget.allow_closing:
                # some system put close button on the left
                for p in (QTabBar.LeftSide, QTabBar.RightSide):
                    bt = self.tabBar().tabButton(i, p)
                    if bt is not None:
                        bt.resize(0, 0)
