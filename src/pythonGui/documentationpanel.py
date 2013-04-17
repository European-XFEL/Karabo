#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the documentation panel inside of
   the configuration panel on the right of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["DocumentationPanel"]


from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtWebKit import *


class DocumentationPanel(QWidget):


    def __init__(self):
        super(DocumentationPanel, self).__init__()
        
        self.__swDocumentation = QStackedWidget()
        # Initial page
        teInitialDescriptionPage = QTextEdit()
        teInitialDescriptionPage.setReadOnly(True)
        self.__swDocumentation.addWidget(teInitialDescriptionPage)
        
        self.__wikiTicket = ""
        self.__wikiView = QWebView()
        self.__wikiView.load(QUrl("https://docs.xfel.eu/alfresco/service/api/login?u=wiki&pw=$w1k1!"))
        #self.__wikiView.loadProgress.connect(self.onLoadProgressWiki)
        self.__wikiView.loadFinished.connect(self.onLoadFinishedWiki)
        
        self.__reportView = QWebView()
        self.__reportView.load(QUrl("http://www-exfel-wp76.desy.de/static/mantisbt/login_select_proj_page.php?ref=bug_report_page.php"))
        #self.__reportView.loadProgress.connect(self.onLoadProgressReport)
        #self.__reportView.loadFinished.connect(self.onLoadFinishedReport)
        
        self.__tabWidget = QTabWidget(self)
        text = "Parameter information"
        index = self.__tabWidget.addTab(self.__swDocumentation, text)
        self.__tabWidget.setTabToolTip(index, text)
        text = "Wiki"
        index = self.__tabWidget.addTab(self.__wikiView, text)
        self.__tabWidget.setTabToolTip(index, text)
        text = "Report problem"
        index = self.__tabWidget.addTab(self.__reportView, text)
        self.__tabWidget.setTabToolTip(index, text)
        self.__tabWidget.currentChanged.connect(self.onCurrentTabChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__tabWidget)
        
        Manager().notifier.signalNavigationItemChanged.connect(self.onNavigationItemChanged)


    def _setWikiActionsVisible(self, visible):
        self.__acBackWiki.setVisible(visible)
        self.__acForwardWiki.setVisible(visible)
        self.__acReloadWiki.setVisible(visible)
        self.__acStopWiki.setVisible(visible)


    def _setReportActionsVisible(self, visible):
        self.__acBackReport.setVisible(visible)
        self.__acForwardReport.setVisible(visible)
        self.__acReloadReport.setVisible(visible)
        self.__acStopReport.setVisible(visible)


    def setupActions(self):
        pass


    def setupToolBar(self, toolBar):
        self.__toolBar = toolBar
        
        self.__acBackWiki = self.__wikiView.pageAction(QWebPage.Back)
        self.__acForwardWiki = self.__wikiView.pageAction(QWebPage.Forward)
        self.__acReloadWiki = self.__wikiView.pageAction(QWebPage.Reload)
        self.__acStopWiki = self.__wikiView.pageAction(QWebPage.Stop)
        
        self.__toolBar.addAction(self.__acBackWiki)
        self.__toolBar.addAction(self.__acForwardWiki)
        self.__toolBar.addAction(self.__acReloadWiki)
        self.__toolBar.addAction(self.__acStopWiki)
        
        self.__acBackReport = self.__reportView.pageAction(QWebPage.Back)
        self.__acForwardReport = self.__reportView.pageAction(QWebPage.Forward)
        self.__acReloadReport = self.__reportView.pageAction(QWebPage.Reload)
        self.__acStopReport = self.__reportView.pageAction(QWebPage.Stop)
        
        self.__toolBar.addAction(self.__acBackReport)
        self.__toolBar.addAction(self.__acForwardReport)
        self.__toolBar.addAction(self.__acReloadReport)
        self.__toolBar.addAction(self.__acStopReport)
        
        self._setWikiActionsVisible(False)
        self._setReportActionsVisible(False)


    def addWidget(self, widget):
        return self.__swDocumentation.addWidget(widget)


    def setCurrentIndex(self, index):
        self.__swDocumentation.setCurrentIndex(index)


### slots ###
    def onCurrentTabChanged(self, index):
        if index == 0:
            self._setWikiActionsVisible(False)
            self._setReportActionsVisible(False)
        elif index == 1:
            self._setWikiActionsVisible(True)
            self._setReportActionsVisible(False)
        elif index == 2:
            self._setWikiActionsVisible(False)
            self._setReportActionsVisible(True)


    def onNavigationItemChanged(self, itemInfo):
        devClaId = itemInfo.get(QString('devClaId'))
        if devClaId is None:
            devClaId = itemInfo.get('devClaId')
        
        if devClaId is None:
            self.__wikiView.load(QUrl("https://docs.xfel.eu/share/page/site-index"))
            return
        
        url = "https://docs.xfel.eu/share/page/site/karabo/device-page?title="+devClaId+"&action=view&parentNodeRef=workspace/SpacesStore/18cffe24-4ce3-4d26-b3c1-fdc953edff59&alf_ticket=" + self.__wikiTicket
        self.__wikiView.load(QUrl(url))


    #def onLoadProgressWiki(self, percent):
    #    print "onLoadProgressWiki", percent

    
    def onLoadFinishedWiki(self, state):
        if len(self.__wikiTicket) == 0:
            self.__wikiTicket =  self.__wikiView.page().currentFrame().toPlainText()
            self.__wikiView.blockSignals(True)
            self.__wikiView.load(QUrl("https://docs.xfel.eu/share/"))
            self.__wikiView.blockSignals(False)


    #def onLoadProgressReport(self, percent):
    #    print "onLoadProgressReport", percent

    
    #def onLoadFinishedReport(self, state):
    #    print "onLoadFinishedReport", state


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

