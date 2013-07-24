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
from PyQt4.QtNetwork import *


class DocumentationPanel(QWidget):


    def __init__(self):
        super(DocumentationPanel, self).__init__()
        
        self.__loadedUrl = str()
        
        self.__postMessage = QByteArray("<form id=\"loginform\" accept-charset=\"UTF-8\" method=\"post\" action=\"https://docs.xfel.eu/share/page/dologin\"><fieldset><input type=\"text\" id=\"username\" name=\"username\" value=\"wiki\" /><input type=\"password\" id=\"password\" name=\"password\" value=\"karabo\"/><input type=\"hidden\" id=\"success\" name=\"success\" value=\"https://docs.xfel.eu/share/page/site/karabo/device-page?title=Conveyor\"/><input type=\"hidden\" name=\"failure\" value=\"https://docs.xfel.eu/share/page/site/karabo/device-page?title=Conveyor&guest=true\"/></fieldset></form>")
                                    
        print "Message: ", self.__postMessage
        
        
        
        # String for wiki ticket
        #self.__wikiTicket = str()
        # Hidden view to fetch ticket
        #self.__hiddenWikiTicketView = QWebView(self)
        
        #url = "https://docs.xfel.eu/alfresco/service/api/login?u=wiki&pw=$w1k1!"
        #self.__hiddenWikiTicketView.load(QUrl(url))
        #self.__hiddenWikiTicketView.setVisible(False)
        
        self.__request = QNetworkRequest(QUrl("https://docs.xfel.eu/share/page/dologin/"))
        self.__request.setRawHeader('Content-Type', QByteArray('application/x-www-form-urlencoded'))
        
        self.__wikiView = QWebView()
        self.__wikiView.loadFinished.connect(self.onLoadFinishedWikiTicket)
        self.__wikiView.load(self.__request, QNetworkAccessManager.PostOperation, self.__postMessage)
        self.__wikiView.show()
        
        #self._loadWikiUrl("https://docs.xfel.eu/share/")
        
        self.__reportView = QWebView()
        self.__reportView.load(QUrl("http://www-exfel-wp76.desy.de/static/mantisbt/login_select_proj_page.php?ref=bug_report_page.php"))
        
        self.__tabWidget = QTabWidget(self)
        
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


    def _loadWikiUrl(self, url):
        if self.__loadedUrl == url:
            return
        
        self.__loadedUrl = url
        self.__wikiView.load(QUrl(self.__loadedUrl))
        self.__wikiView.show()


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
        
        self._setWikiActionsVisible(True)
        self._setReportActionsVisible(False)


### slots ###
    def onCurrentTabChanged(self, index):
        if index == 0:
            self._setWikiActionsVisible(True)
            self._setReportActionsVisible(False)
        elif index == 1:
            self._setWikiActionsVisible(False)
            self._setReportActionsVisible(True)


    def onNavigationItemChanged(self, itemInfo):
        classId = itemInfo.get(QString('classId'))
        if classId is None:
            classId = itemInfo.get('classId')
        
        if classId is None:
            self._loadWikiUrl("https://docs.xfel.eu/share/")
            return
        
        url = "https://docs.xfel.eu/share/page/site/karabo/device-page?title="+classId+"&action=view&parentNodeRef=workspace/SpacesStore/18cffe24-4ce3-4d26-b3c1-fdc953edff59&alf_ticket=" + self.__wikiTicket
        self._loadWikiUrl(url)
    
    
    def onLoadFinishedWikiTicket(self, state):
        print "STATE: ", state
        return
        if len(self.__wikiTicket) > 0:
            return
        
        self.__wikiTicket = self.__hiddenWikiTicketView.page().currentFrame().toPlainText()
        # Disconnect slot after fetching ticket for Wiki
        self.__hiddenWikiTicketView.loadFinished.disconnect(self.onLoadFinishedWikiTicket)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

