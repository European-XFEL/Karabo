#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the documentation panel inside of
   the configuration panel on the right of the MainWindow which is un/dockable.
"""

__all__ = ["DocumentationPanel"]


from PyQt4.QtCore import QUrl
from PyQt4.QtGui import QTabWidget, QVBoxLayout, QWidget
from PyQt4.QtWebKit import QWebPage, QWebView


class DocumentationPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    #
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################


    def __init__(self):
        super(DocumentationPanel, self).__init__()

        self.reportView = QWebView()
        self.reportView.load(QUrl("https://in.xfel.eu/redmine"))
        
        self.tabWidget = QTabWidget(self)
        
        text = "Report problem"
        index = self.tabWidget.addTab(self.reportView, text)
        self.tabWidget.setTabToolTip(index, text)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.tabWidget)


    def _setReportActionsVisible(self, visible):
        self.acBackReport.setVisible(visible)
        self.acForwardReport.setVisible(visible)
        self.acReloadReport.setVisible(visible)
        self.acStopReport.setVisible(visible)


    def setupActions(self):
        pass


    def setupToolBars(self, toolBar, parent):
        self.acBackReport = self.reportView.pageAction(QWebPage.Back)
        self.acForwardReport = self.reportView.pageAction(QWebPage.Forward)
        self.acReloadReport = self.reportView.pageAction(QWebPage.Reload)
        self.acStopReport = self.reportView.pageAction(QWebPage.Stop)
        
        toolBar.addAction(self.acBackReport)
        toolBar.addAction(self.acForwardReport)
        toolBar.addAction(self.acReloadReport)
        toolBar.addAction(self.acStopReport)
        
        self._setReportActionsVisible(True)


### slots ###
    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

