#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on September 18, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayPyCodeHist"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *

#iPython ZMQ Kernel manager utilities
#from IPython.frontend.qt.kernelmanager import QtKernelManager
from IPython.qt.manager import QtKernelManager
#from IPython.frontend.qt.svg import svg_to_image
from IPython.qt.svg import svg_to_image
from base64 import decodestring

import time
import thread
import datetime


class DisplayPyCodeHist(DisplayWidget):
    category = "Digit"
    alias = "Matplotlib Plot"

    def __init__(self, box, parent):
        super(DisplayPyCodeHist, self).__init__(box)
        
        self.__compositeWidget = QWidget(parent)
        vLayout = QVBoxLayout(self.__compositeWidget)
        vLayout.setContentsMargins(0,0,0,0)
        
        #a rich text edit to output textual output from the kernel
        self.__response = QTextEdit()
        self.__response.setAcceptRichText(True)
        self.__response.setMinimumSize(160, 30)
        self.__response.setEnabled(True)
        vLayout.addWidget(self.__response)
        
        #holds and image results, i.e. plots from the kernel
        self.__response_image = QLabel()
        vLayout.addWidget(self.__response_image)
        
        #a slider to select center of data
        text = "Set data center"
        self.__tbData_pos_slider = QSlider(Qt.Horizontal)
        self.__tbData_pos_slider.setStatusTip(text)
        self.__tbData_pos_slider.setToolTip(text)
        self.__tbData_pos_slider.setMinimum(0);
        self.__tbData_pos_slider.setMaximum(100);
        self.__tbData_pos_slider.setTickInterval(10);
        self.__tbData_pos_slider.setSingleStep(1);
        self.__tbData_pos_slider.setValue(100);
        self.__tbData_pos_slider.valueChanged.connect(self.onDataSliderChanged)
        vLayout.addWidget(self.__tbData_pos_slider)
        
        self.__slider_pos = 100
        
        #zoom controls
        self.__zoomWidget = QWidget()
        hZoomLayout = QHBoxLayout(self.__zoomWidget)
        hZoomLayout.setContentsMargins(0,0,0,0)
        text = "Zoom In"
        self.__tbZoomIn = QToolButton()
        self.__tbZoomIn.setStatusTip(text)
        self.__tbZoomIn.setToolTip(text)
        self.__tbZoomIn.setIconSize(QSize(24,24))
        self.__tbZoomIn.setIcon(QIcon(":transform-scale-up"))
        self.__tbZoomIn.setEnabled(True)
        self.__tbZoomIn.clicked.connect(self.onZoomIn)
        hZoomLayout.addWidget(self.__tbZoomIn)
        
        text = "Zoom Out"
        self.__tbZoomOut = QToolButton()
        self.__tbZoomOut.setStatusTip(text)
        self.__tbZoomOut.setToolTip(text)
        self.__tbZoomOut.setIconSize(QSize(24,24))
        self.__tbZoomOut.setIcon(QIcon(":transform-scale-down"))
        self.__tbZoomOut.setEnabled(True)
        self.__tbZoomOut.clicked.connect(self.onZoomOut)
        hZoomLayout.addWidget(self.__tbZoomOut)
        
        vLayout.addWidget(self.__zoomWidget)
        self.__zoomFactor = 1
        
        
        #hide button - hides code box and tool bars
        text = "Toogle code panel"
        self.__tbTogglePanel = QToolButton()
        self.__tbTogglePanel.setStatusTip(text)
        self.__tbTogglePanel.setToolTip(text)
        self.__tbTogglePanel.setIconSize(QSize(24,24))
        self.__tbTogglePanel.setIcon(QIcon(":configure"))
        self.__tbTogglePanel.setEnabled(True)
        # Use action for button to reuse
        self.__acTogglePanel = QAction(QIcon(":configure"), text, self)
        self.__acTogglePanel.setStatusTip(text)
        self.__acTogglePanel.setToolTip(text)
        self.__acTogglePanel.setEnabled(True)
        self.__acTogglePanel.triggered.connect(self.onTogglePanelClicked)
        self.__tbTogglePanel.clicked.connect(self.__acTogglePanel.triggered)
        self.__panelVisible = True
        # Add to layout
        vLayout.addWidget(self.__tbTogglePanel)
        
        
        #the code tools field
        self.__codeToolWidget = QWidget()
        vCodeLayout = QVBoxLayout(self.__codeToolWidget)
        vCodeLayout.setContentsMargins(0,0,0,0)
        
        #the code input box
        self.__lineEdit = QTextEdit()
        self.__lineEdit.setAcceptRichText(False)
        self.__lineEdit.setMinimumSize(160, 160)
        self.__lineEdit.setEnabled(True)
        vCodeLayout.addWidget(self.__lineEdit)
        
        
        #json edit field
        #type new to create a new kernel or paste the location of the connection
        #json file of an existing kernel
        self.__ipEditLabel = QLabel('Kernel: "new" or connection JSON file')
        vCodeLayout.addWidget(self.__ipEditLabel)
        
        self.__ipEdit = QLineEdit()
        self.__ipEdit.setMinimumSize(160,20)
        self.__ipEdit.setEnabled(True)
        self.__ipEdit.setText('new')
        vCodeLayout.addWidget(self.__ipEdit)
        
        
        #h layout for buttons
        self.__buttonBar = QWidget()
        hLayout = QHBoxLayout(self.__buttonBar)
        hLayout.setContentsMargins(0,0,0,0)
        
        
        #sets the refresh rate for the plots
        #type no to no refresh at all
        self.__refreshEditLabel = QLabel('Refresh: rate (Hz) or "no"')
        vCodeLayout.addWidget(self.__refreshEditLabel)
        
        self.__refreshEdit = QLineEdit()
        self.__refreshEdit.setMinimumSize(160,20)
        self.__refreshEdit.setEnabled(True)
        self.__refreshEdit.setText('no')
        hLayout.addWidget(self.__refreshEdit)
        self.__refresh_rate = 10 #Hz
        self.__should_refresh = False
        self.connect(self, SIGNAL('clearResponse'), self.clearResponseContents )
        
        
        #execute button - starts refreshing if this is set
        text = "Execute"
        self.__tbExecute = QToolButton()
        self.__tbExecute.setStatusTip(text)
        self.__tbExecute.setToolTip(text)
        self.__tbExecute.setIconSize(QSize(24,24))
        self.__tbExecute.setIcon(QIcon(":refresh"))
        self.__tbExecute.setEnabled(True)
        # Use action for button to reuse
        self.__acExecute = QAction(QIcon(":refresh"), text, self)
        self.__acExecute.setStatusTip(text)
        self.__acExecute.setToolTip(text)
        self.__acExecute.setEnabled(True)
        self.__acExecute.triggered.connect(self.onExecuteClicked)
        self.__tbExecute.clicked.connect(self.__acExecute.triggered)
        # Add to layout
        hLayout.addWidget(self.__tbExecute)
        #self.__busy_lock = allocate_lock()
        
        #a button to stop it from refreshing
        text = "Stop"
        self.__tbStop = QToolButton()
        self.__tbStop.setStatusTip(text)
        self.__tbStop.setToolTip(text)
        self.__tbStop.setIconSize(QSize(24,24))
        self.__tbStop.setIcon(QIcon(":stop"))
        self.__tbStop.setEnabled(False)
        # Use action for button to reuse
        self.__acStop = QAction(QIcon(":stop"), text, self)
        self.__acStop.setStatusTip(text)
        self.__acStop.setToolTip(text)
        self.__acStop.setEnabled(True)
        self.__acStop.triggered.connect(self.onStopClicked)
        self.__tbStop.clicked.connect(self.__acStop.triggered)
        # Add to layout
        hLayout.addWidget(self.__tbStop)
        
        
        vCodeLayout.addWidget(self.__buttonBar)
        
        #h layout for checkboxes
        self.__checkBoxBar = QWidget()
        hcLayout = QHBoxLayout(self.__checkBoxBar)
        hcLayout.setContentsMargins(0,0,0,0)
        
        #checkbox for showing python output
        self.__showPyOutLabel = QLabel('Show PyOut:')
        hcLayout.addWidget(self.__showPyOutLabel)
        
        #should the py output be shown
        text = "Show PyOut"
        self.__tbPyOutCheck = QCheckBox()
        self.__tbPyOutCheck.setStatusTip(text)
        self.__tbPyOutCheck.setToolTip(text)
        self.__tbPyOutCheck.setEnabled(True)
        self.__tbPyOutCheck.setChecked(True)
        self.__tbPyOutCheck.stateChanged.connect(self.onPyOutCheckChange)
        hcLayout.addWidget(self.__tbPyOutCheck)
        
        #checkbox for showing python output
        self.__showUpdateOnCodeLabel = QLabel('Update on code:')
        hcLayout.addWidget(self.__showUpdateOnCodeLabel)
        
        #should the py output be shown
        text = "Update on code change"
        self.__tbUpdateOnCodeCheck = QCheckBox()
        self.__tbUpdateOnCodeCheck.setStatusTip(text)
        self.__tbUpdateOnCodeCheck.setToolTip(text)
        self.__tbUpdateOnCodeCheck.setEnabled(True)
        self.__tbUpdateOnCodeCheck.setChecked(True)
        self.__tbUpdateOnCodeCheck.stateChanged.connect(self.onUpdateOnCodeCheckChange)
        hcLayout.addWidget(self.__tbUpdateOnCodeCheck)
        self.__auto_update_on_code = True
        
        
        vCodeLayout.addWidget(self.__checkBoxBar)
        
        vLayout.addWidget(self.__codeToolWidget);
        
        #init kernel managment class
        self.__kernel_manager_class = QtKernelManager
        self.__kernel_initiated = False
        
        #keep hold of messages from the kernel
        self.__execute_messages = []
        self.__max_messages = 5 #maximum number of message to hold in a queue
        
        self.__data_structure_initialized = False
        self.__plot_code = []
        self.__base_time_for_utc = datetime.datetime(01,01,01,00,00,00)
        self.__initialized = True
        
        self.__minTimeBound = self.TimeSpanToFloat(datetime.datetime.now() - self.__base_time_for_utc)
        self.__maxTimeBound = self.TimeSpanToFloat(datetime.datetime.now() - self.__base_time_for_utc)
        
        #self.executing = QtCore.Signal(object)
        #self.executed = QtCore.Signal(object)


    def TimeSpanToFloat(self, dt):
        return dt.days+dt.seconds/(3600.0*24.0)+dt.microseconds/(1E6*3600.0*24.0)
        

    #initiate a new kernal on local host
    #mostly taken from iPython Qtconsole code
    def initiateNewKernel(self):
        ip = '127.0.0.1'
        kernel_manager = self.__kernel_manager_class(
                                ip=ip,
        )
        # start the kernel
        kwargs = dict()
        kernel_manager.start_kernel(**kwargs)
        kernel_manager.start_channels()
        return kernel_manager
    
    #connect to an existing kernel via a json file
    #mostly taken from iPython Qtconsole code
    def connectToExistingKernel(self, connection_file):
        kernel_manager = self.__kernel_manager_class(
                                connection_file=connection_file,
        )
        kernel_manager.load_connection_file()
        kernel_manager.start_channels()
        return kernel_manager
    
    #check if this message was sent from this session
    #mostly taken from iPython Qtconsole code
    def _is_from_this_session(self, msg):
        """ Returns whether a reply from the kernel originated from a request
            from this frontend.
        """
        session = self.__kernel_manager.session.session
        parent = msg['parent_header']
        if not parent:
            # if the message has no parent, assume it is meant for all frontends
            return True
        else:
            return parent.get('session') == session

    #toggles code panel visibility
    def onTogglePanelClicked(self):
        if self.__panelVisible is True:
            self.__panelVisible = False
            
            self.__lineEdit.setMinimumSize(160, 0)
        else:
            self.__panelVisible = True
            
            self.__lineEdit.setMinimumSize(160, 160)
        
        self.__codeToolWidget.setVisible(self.__panelVisible)
        self.__codeToolWidget.layout().activate()
        self.__compositeWidget.layout().activate()
        

    def onPyOutCheckChange(self):
        if self.__tbPyOutCheck.isChecked() is True:
            self.__response.setVisible(True)
            self.__response.setMinimumSize(160, 0)
        else:
            self.__response.setVisible(False)
            self.__response.setMinimumSize(160, 30)

        self.__codeToolWidget.layout().activate()
        self.__compositeWidget.layout().activate()
        
    def onUpdateOnCodeCheckChange(self):
        if self.__tbUpdateOnCodeCheck.isChecked() is True:
            self.__auto_update_on_code = True
        else:
            self.__auto_update_on_code = False
    

    def initKernel(self):
        #check if we should connect to an existing kernel
        kernel_manager = None
        if str(self.__ipEdit.text()) == "new":
            kernel_manager = self.initiateNewKernel()
        else:
            kernel_manager = self.connectToExistingKernel(str(self.__ipEdit.text()))
        self.__kernel_manager = kernel_manager

        #connect dispatch channels
        kernel_manager.sub_channel.message_received.connect(self._dispatch)
        kernel_manager.shell_channel.message_received.connect(self._dispatch)
        kernel_manager.stdin_channel.message_received.connect(self._dispatch)

        #execute pylab inline so that figures will be inline
        code = []
        code.append("%pylab inline")
        code.append("from karabo.deviceClient import *")
        code.append("history_values = dict()")
        
        msg_id = self.__kernel_manager.shell_channel.execute("\n".join(code), False)
        self.__kernel_initiated = True

    #handle execution requests
    #if no kernels have been started do this first
    #if no refresh is requested execute once else start an execution thread
    def onExecuteClicked(self):
        #ret  = eval(str(self.__lineEdit.toPlainText()))
        #if the kernel hasn't been initiated to this now
        if self.__kernel_initiated is False:
            self.initKernel()
   
        #after starting kernels get the source text to execute
        source = str(self.__lineEdit.toPlainText())
        if len(source) == 0:
            return
        
        source += "\n"
        
        #activate stop executing button
        self.__tbStop.setEnabled(True)
        self.__tbExecute.setEnabled(False)
        
        self.emit(SIGNAL('clearResponse'))
        if str(self.__refreshEdit.text()) == "no":
            self.__should_refresh = False
             #clear the response field
            
            msg_id = self.__kernel_manager.shell_channel.execute(source, False)       
            self.__execute_messages.append(msg_id)
            self.__tbStop.setEnabled(False)
            self.__tbExecute.setEnabled(True)
        else:
            refresh_rate = float(str(self.__refreshEdit.text()))
            self.__should_refresh = True
            self.__refresh_rate = refresh_rate
            #call execute in a separate thread
            thread.start_new_thread(self.refreshedExecute, (source,))
            
    #clear the response field before new executions
    def clearResponseContents(self):
        self.__response.clear()
    
    #function to start threaded for a refreshed execution
    def refreshedExecute(self, source):
        while self.__should_refresh:
            #only execute if we have less than max messages in the queue
            
            msg_id = self.__kernel_manager.shell_channel.execute(source, False)
            time.sleep(1/self.__refresh_rate)
        
    def onStopClicked(self):
        self.__tbStop.setEnabled(False)
        self.__should_refresh = False
        self.__tbExecute.setEnabled(True)
        
    def _dispatch(self, msg):
        msg_id = msg['parent_header']['msg_id']
        session_id = msg['parent_header']['session']
        #only process message which are from self
        if not self._is_from_this_session(msg):
            return
        msg_type = msg['header']['msg_type']
        

        if msg_type == "pyout" or msg_type =="display_data":
            self._process_pyout_reply(msg)
        elif msg_type == "pyerr":
            self._process_pyerr_reply(msg)
        elif msg_type == "stream":
            self._process_pystream_reply(msg)    
            
        
        
    #std output for python provided output    
    def _process_pyout_reply(self,msg):
        content = msg['content']
        
        data = content['data']
        if data.has_key('text/html'):
            html = data['text/html']
            self.__response.setTextColor(QColor("black"))
            self.__response.setHtml(html)      
        elif data.has_key('text/plain'):
            text = data['text/plain']
            self.__response.setTextColor(QColor("black"))
            self.__response.setPlainText(text)
        elif data.has_key('image/png'):
            self._append_png(decodestring(data['image/png'].encode('ascii')))
        elif data.has_key('image/svg+xml'):
            self._append_svg(data['image/svg+xml'], True)
            
    
    #std error output
    def _process_pyerr_reply(self,msg):
        content = msg['content']
        
        evalue = content['evalue']
        self.__response.setTextColor(QColor("red"))
        self.__response.setPlainText(evalue)
        
    
    #std output for printed data, e.g. user initiated output
    def _process_pystream_reply(self,msg):
        content = msg['content']
        data = content['data']
        self.__response.setTextColor(QColor("black"))
        self.__response.setPlainText(data)
        
   
    def _append_png(self, png):
        """ Insert raw PNG data into the widget.
        """
        self._append_img(png, 'png')

    def _append_img(self, img, fmt):
        """ insert a raw image, png """
        try:
            image = QImage()
            image.loadFromData(img, fmt.upper())
            pixmap = QPixmap.fromImage(image)
            self.__response_image.setPixmap(pixmap)
            
        except ValueError:
            self.__response.setTextColor(QColor("red"))
            self.__response.setPlainText('Received invalid %s data.'%fmt)
            
        #else:
            
            #format = self._add_image(image)
            #cursor = self.__response.textCursor()
            #cursor.insertBlock()
            #cursor.insertImage(format)
            #cursor.insertBlock()

    def _append_svg(self, svg):
        """ Insert raw SVG data into the widet.
        """
        try:
            image = svg_to_image(svg)
            pixmap = QPixmap.fromImage(image)
            self.__response_image.setPixmap(pixmap)
        except ValueError:
            self._insert_plain_text(cursor, 'Received invalid SVG data.')
        #else:
            #format = self._add_image(image)
            #cursor = self.__response.textCursor()
            #cursor.insertBlock()
            #cursor.insertImage(format)
            #cursor.insertBlock()
            
    def _add_image(self, image):
        """ Adds the specified QImage to the document and returns a
            QTextImageFormat that references it.
        """
        document = self.__response.document()
        name = str(image.cacheKey())
        document.addResource(QTextDocument.ImageResource,
                             QUrl(name), image)
        format = QTextImageFormat()
        format.setName(name)
        return format

    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lineEdit.toPlainText()


    def addInitCode(self, code, key):
        code.append("#-----do not remove the following lines-------")
        code.append("import matplotlib.pyplot as plt")
        code.append("import matplotlib.dates as mdates")
        
        code.append("if not'"+key+"' in history_values:")
        code.append("   history_values['"+key+"'] = dict()")
        code.append("   history_values['"+key+"']['val'] = []")
        code.append("   history_values['"+key+"']['time'] = []")
        code.append("   history_values['"+key+"']['xmin']="+str(self.__minTimeBound))
        code.append("   history_values['"+key+"']['xmax']="+str(self.__maxTimeBound))
        code.append("#-----do not remove the above lines-------")
        return code
    
    
    def addPlotCode(self, plotting_code, key):
        plotting_code.append("#-----code describing plot, edit if needed-------")
        plotting_code.append("fmt = mdates.DateFormatter('%Y-%m-%d %H:%M:%S')")
        plotting_code.append("ax = plt.axes()")
        plotting_code.append("ax.xaxis.set_major_formatter(fmt)")
        plotting_code.append("ax.set_xlim(history_values['"+key+"']['xmin'], history_values['"+key+"']['xmax'])")
        plotting_code.append("plt.plot_date(x=history_values['"+key+"']['time'], y=history_values['"+key+"']['val'], fmt='r-')")
        plotting_code.append("plt.title('"+key+"')")
        plotting_code.append("plt.grid(True)")
        plotting_code.append("fig = plt.figure(1)")
        plotting_code.append("fig.autofmt_xdate()")
        plotting_code.append("plt.show()")
        return plotting_code


    def valueChanged(self, key, value, timestamp=None):
        
        #if device hasn't initialized yet return
        try:
            if self.__initialized is True:
                pass
        except:
            return
                
        
        if value is None:
            return

        if timestamp is None:
            #generate timestap from now
            dt = datetime.datetime.now() - self.__base_time_for_utc
            timestamp = self.TimeSpanToFloat(dt)
        
        #if we are at the maximum slider position adjust maxTimeBound
        if self.__slider_pos == 100:
            self.__maxTimeBound = timestamp
        
        if self.__kernel_initiated is False:
            return
        
        if self.__data_structure_initialized is False:
            code = []
            code = self.addInitCode(code, str(key))
            ex_msg = "\n".join(code)
            msg_id = self.__kernel_manager.shell_channel.execute(ex_msg, False)
            self.__data_structure_initialized = True
            
            plotting_code = []
            plotting_code = self.addPlotCode(plotting_code, str(key))
            self.__plot_code = plotting_code
            self.__lineEdit.blockSignals(True)
            plt_txt = "\n".join(plotting_code)
            self.__lineEdit.setPlainText(plt_txt)
            self.__lineEdit.blockSignals(False)
            #restart executing
            self.onStopClicked()
            self.onExecuteClicked();
            
            
        
        #append newest value
        code = []
        code.append("history_values['"+str(key)+"']['val'].append("+str(value)+")")
        code.append("history_values['"+str(key)+"']['time'].append("+str(timestamp)+")")
        code.append("history_values['"+str(key)+"']['xmin']="+str(self.__minTimeBound))
        code.append("history_values['"+str(key)+"']['xmax']="+str(self.__maxTimeBound))
        msg_id = self.__kernel_manager.shell_channel.execute("\n".join(code), False)
        
                    
    def onDataSliderChanged(self):
        self.__slider_pos = self.__tbData_pos_slider.tickPosition()
 
    def onZoomIn(self):
        self.__zoomFactor *= 2.0
        self.calculateAndSetRanges()
        
    def onZoomOut(self):
        self.__zoomFactor /= 2.0
        self.calculateAndSetRanges()
        
    def calculateAndSetRanges(self):
        dt = self.__maxTimeBound - self.__minTimeBound
        center = self.__slider_pos/100.0*dt+self.__minTimeBound
        dtzoomed = dt/self.__zoomFactor
        min = center-dtzoomed/2.0
        if min < 0:
            min = 0
        self.__minTimeBound = min
        self.__maxTimeBound = center+dtzoomed/2
        print center
        print self.__minTimeBound
        print self.__maxTimeBound
        
