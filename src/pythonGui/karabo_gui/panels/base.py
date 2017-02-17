#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import (QAction, QFrame, QHBoxLayout, QSizePolicy,
                         QVBoxLayout, QWidget)

from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               broadcast_event)
import karabo_gui.icons as icons
from karabo_gui.toolbar import ToolBar
from karabo_gui.util import generateObjectName


class BasePanelWidget(QFrame):
    """The base class for all panels shown in the GUI. Panels can either be
    independent top-level windows or tabs within the main window of the Karabo
    gui.
    """
    def __init__(self, container, title):
        super(BasePanelWidget, self).__init__(parent=None)

        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setLineWidth(1)

        self.index = -1
        self.doesDockOnClose = True
        self.panel_container = container  # tab widget
        self.updateTitle(title)

        self._fill_panel()

    # --------------------------------------
    # BasePanelWidget interface

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        raise NotImplementedError

    def dock(self):
        """Called when this panel is docked into the main window.
        """

    def undock(self):
        """Called when this panel is undocked from the main window.
        """

    def toolbars(self):
        """This should create and return a list of `ToolBar` instances needed
        by this panel.
        """
        return []

    # --------------------------------------
    # public methods

    def forceClose(self):
        """
        This function sets the member variable to False which decides whether
        this widget should be closed on dockonevent and calls closeEvent.
        """
        self.doesDockOnClose = False
        return self.close()

    def updateTitle(self, title):
        """ The title of the widget changed and needs to be updated.
        """
        self.title = title
        self.setWindowTitle(self.title)

    # --------------------------------------
    # Qt slots and callbacks

    def closeEvent(self, event):
        if self.doesDockOnClose:
            self.onDock()
            event.ignore()
        else:
            if self.close():
                event.accept()
            else:
                event.ignore()

    def onUndock(self):
        self.acDock.setVisible(True)
        self.acUndock.setVisible(False)
        self.setWindowTitle(self.title)
        self.undock()
        self.panel_container.undock(self)

    def onDock(self):
        self.acDock.setVisible(False)
        self.acUndock.setVisible(True)
        self.dock()
        self.panel_container.dock(self)

    def onMaximize(self):
        self.acMinimize.setVisible(True)
        self.acMaximize.setVisible(False)

        i = self.panel_container.count()
        while i > -1:
            i -= 1
            w = self.panel_container.widget(i)
            if w != self:
                self.panel_container.removeTab(i)

        d = {'tab': self.panel_container}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.MaximizeTab, d))

    def onMinimize(self):
        self.acMinimize.setVisible(False)
        self.acMaximize.setVisible(True)

        for w in self.panel_container.panel_set:
            if w == self:
                continue

            self.panel_container.insertTab(w.index, w, w.title)

        d = {'tab': self.panel_container}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.MinimizeTab, d))

    # --------------------------------------
    # private methods

    def _fill_panel(self):
        # Do this first
        main_content = self.get_content_widget()

        # Then build the toolbar container
        toolbar = QWidget(self)
        toolbar_layout = QHBoxLayout(toolbar)
        all_toolbars = [self._standard_toolbar()] + self.toolbars()
        for tb in all_toolbars:
            toolbar_layout.addWidget(tb)
        toolbar_layout.setContentsMargins(0, 0, 0, 0)
        toolbar_layout.setSpacing(0)
        # Make the last toolbar expand to fill all horizontal space
        toolbar_layout.setStretch(toolbar_layout.count()-1, 1)

        # Setup some visual characteristics of the toolbar container
        stylesheet = 'QWidget#{} {{background-color: rgb(180,180,180); }}'
        name = generateObjectName(toolbar)
        toolbar.setObjectName(name)
        toolbar.setStyleSheet(stylesheet.format(name))
        toolbar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # Save a reference so that subclasses can query its properties
        self.toolbar = toolbar

        main_layout = QVBoxLayout(self)
        main_layout.addWidget(self.toolbar)
        main_layout.addWidget(main_content)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

    def _standard_toolbar(self):
        """This toolbar is added to all panels
        """
        text = "Unpin as individual window"
        self.acUndock = QAction(icons.undock, "&Undock", self)
        self.acUndock.setToolTip(text)
        self.acUndock.setStatusTip(text)
        self.acUndock.triggered.connect(self.onUndock)

        text = "Pin this window to main program"
        self.acDock = QAction(icons.dock, "&Dock", self)
        self.acDock.setToolTip(text)
        self.acDock.setStatusTip(text)
        self.acDock.triggered.connect(self.onDock)
        self.acDock.setVisible(False)

        text = "Maximize panel"
        self.acMaximize = QAction(icons.maximize, "&Maximize", self)
        self.acMaximize.setToolTip(text)
        self.acMaximize.setStatusTip(text)
        self.acMaximize.triggered.connect(self.onMaximize)

        text = "Minimize panel"
        self.acMinimize = QAction(icons.minimize, "&Minimize", self)
        self.acMinimize.setToolTip(text)
        self.acMinimize.setStatusTip(text)
        self.acMinimize.triggered.connect(self.onMinimize)
        self.acMinimize.setVisible(False)

        toolbar = ToolBar("Standard", parent=self)
        toolbar.addAction(self.acUndock)
        toolbar.addAction(self.acDock)
        toolbar.addAction(self.acMaximize)
        toolbar.addAction(self.acMinimize)
        return toolbar
