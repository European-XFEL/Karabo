#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on March 19, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from __future__ import absolute_import, division
__all__ = ["DisplayImageStack"]

from widget import DisplayWidget
from thread import start_new_thread, allocate_lock
import time
import datetime
import copy
import icons
from karabo import enums

import numpy as np
from guiqwt.plot import ImageDialog, CurveDialog
from guiqwt.image import ImagePlot, ImageItem, TrImageItem
from guiqwt.colormap import *
from guiqwt.curve import CurvePlot
from guiqwt.builder import make
from guiqwt.styles import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def _returnRawByteWidth(cs):
    _, l, _ = enums.ChannelSpaceType[cs].split("_")
    return int(l) // 8


def _returnRawArray(data, dimX, dimY, cs):
    s, l, _ = enums.ChannelSpaceType[cs].split("_")
    t = getattr(np, dict(s="int", u="uint", f="float")[s] + l)
    return np.frombuffer(data, t).reshape((dimX, dimY))


#from https://www.mail-archive.com/pyqt@riverbankcomputing.com/msg22889.html
class RangeSlider(QSlider):
    """ A slider for ranges.

        This class provides a dual-slider for ranges, where there is a defined
        maximum and minimum, as is a normal slider, but instead of having a
        single slider value, there are 2 slider values.

        This class emits the same signals as the QSlider base class, with the
        exception of valueChanged
    """
    def __init__(self, *args):
        super(RangeSlider, self).__init__(*args)

        self._low = self.minimum()
        self._high = self.maximum()

        self.pressed_control = QStyle.SC_None
        self.hover_control = QStyle.SC_None
        self.click_offset = 0

        # 0 for the low, 1 for the high, -1 for both
        self.active_slider = 0

    def low(self):
        return self._low

    def setLow(self, low):
        self._low = low
        self.update()

    def high(self):
        return self._high

    def setHigh(self, high):
        self._high = high
        self.update()


    def paintEvent(self, event):
        # based on http://qt.gitorious.org/qt/qt/blobs/master/src/gui/widgets/qslider.cpp

        painter = QPainter(self)
        style = self.style()

        for i, value in enumerate([self._low, self._high]):
            opt = QStyleOptionSlider()
            self.initStyleOption(opt)

            # Only draw the groove for the first slider so it doesn't get drawn
            # on top of the existing ones every time
            if i == 0:
                opt.subControls = style.SC_SliderGroove | style.SC_SliderHandle
            else:
                opt.subControls = style.SC_SliderHandle

            if self.tickPosition() != self.NoTicks:
                opt.subControls |= style.SC_SliderTickmarks

            if self.pressed_control:
                opt.activeSubControls = self.pressed_control
                opt.state |= style.State_Sunken
            else:
                opt.activeSubControls = self.hover_control

            opt.sliderPosition = value
            opt.sliderValue = value
            style.drawComplexControl(style.CC_Slider, opt, painter, self)


    def mousePressEvent(self, event):
        event.accept()

        style = self.style()
        button = event.button()

        # In a normal slider control, when the user clicks on a point in the 
        # slider's total range, but not on the slider part of the control the
        # control would jump the slider value to where the user clicked.
        # For this control, clicks which are not direct hits will slide both
        # slider parts

        if button:
            opt = QStyleOptionSlider()
            self.initStyleOption(opt)

            self.active_slider = -1

            for i, value in enumerate([self._low, self._high]):
                opt.sliderPosition = value
                hit = style.hitTestComplexControl(style.CC_Slider, opt,
                                                  event.pos(), self)
                if hit == style.SC_SliderHandle:
                    self.active_slider = i
                    self.pressed_control = hit

                    self.triggerAction(self.SliderMove)
                    self.setRepeatAction(self.SliderNoAction)
                    self.setSliderDown(True)
                    break

            if self.active_slider < 0:
                self.pressed_control = QStyle.SC_SliderHandle
                self.click_offset = self.__pixelPosToRangeValue(
                                        self.__pick(event.pos()))
                self.triggerAction(self.SliderMove)
                self.setRepeatAction(self.SliderNoAction)
        else:
            event.ignore()

    def mouseMoveEvent(self, event):
        if self.pressed_control != QStyle.SC_SliderHandle:
            event.ignore()
            return

        event.accept()
        new_pos = self.__pixelPosToRangeValue(self.__pick(event.pos()))
        opt = QStyleOptionSlider()
        self.initStyleOption(opt)

        if self.active_slider < 0:
            offset = new_pos - self.click_offset
            self._high += offset
            self._low += offset
            if self._low < self.minimum():
                diff = self.minimum() - self._low
                self._low += diff
                self._high += diff
                new_pos += diff
            if self._high > self.maximum():
                diff = self.maximum() - self._high
                self._low += diff
                self._high += diff
                new_pos += diff
        elif self.active_slider == 0:
            if new_pos >= self._high:
                new_pos = self._high - 1
            self._low = new_pos
        else:
            if new_pos <= self._low:
                new_pos = self._low + 1
            self._high = new_pos

        self.click_offset = new_pos

        self.update()
        self.emit(SIGNAL('sliderMoved(int,int)'), new_pos, self.active_slider)

    def mouseReleaseEvent(self, event):
        event.accept()
        self.emit(SIGNAL('sliderReleased(int,int)'), self.click_offset,
                    self.active_slider)

    def __pick(self, pt):
        if self.orientation() == Qt.Horizontal:
            return pt.x()
        else:
            return pt.y()


    def __pixelPosToRangeValue(self, pos):
        opt = QStyleOptionSlider()
        self.initStyleOption(opt)
        style = self.style()

        gr = style.subControlRect(style.CC_Slider, opt,
                                  style.SC_SliderGroove, self)
        sr = style.subControlRect(style.CC_Slider, opt,
                                  style.SC_SliderHandle, self)

        if self.orientation() == Qt.Horizontal:
            slider_length = sr.width()
            slider_min = gr.x()
            slider_max = gr.right() - slider_length + 1
        else:
            slider_length = sr.height()
            slider_min = gr.y()
            slider_max = gr.bottom() - slider_length + 1

        return style.sliderValueFromPosition(
            self.minimum(), self.maximum(), pos-slider_min,
            slider_max-slider_min, opt.upsideDown)


class ImagePlotItem(ImagePlot):

    def __init__(self, **params):
        #define view of parent
        self.__gridParam = make.gridparam(major_enabled=(False, False),
                                          minor_enabled=(False, False))
        super(ImagePlotItem, self).__init__(gridparam=self.__gridParam,
                                            **params)

        self.__imageDialogWidget = None
        self.__image = None
        self.__dialogPlot = None
        self.__imageData = None
        self.__dialogImage  = None
        self.__LUTrange = None
        self.__ColorMap = 'jet'
        self.disable_autoscale()

        self.connect(self, SIGNAL("updateColorMap(PyQt_PyObject)"),
                     self.updateColorMap)
        self.connect(self, SIGNAL("updateLUTRange(PyQt_PyObject)"),
                     self.updateLUTRange)


    def mousePressEvent(self, me):
        if(me.modifiers() and Qt.ShiftModifier):
            if self.__imageDialogWidget is None:
                self.__imageDialogWidget = ImageDialog(edit=False, toolbar=True)
                self.__dialogPlot = self.__imageDialogWidget.get_plot()
                self.__dialogPlot.add_item(self.__dialogImage)
            self.setImage(self.__imageData)
            self.__imageDialogWidget.show()

    def setImageDialogWidget(self, idw):
        self.__imageDialogWidget = idw


    def updateLUTRange(self, r):
        self.__LUTrange = r

        if self.__image is not None:
            #self.__image.set_lut_range(self.__LUTrange)
            self.__image.set_data(self.__imageData, lut_range=self.__LUTrange)

        if self.__dialogPlot is not None:
            self.__dialogImage.set_data(self.__imageData,
                                        lut_range=self.__LUTrange)


    def updateColorMap(self, map):
        self.__ColorMap = map
        if self.__image is not None:
            self.__image.set_color_map(self.__ColorMap)
            self.replot()
        if self.__dialogPlot is not None:
            self.__dialogImage.set_color_map(self.__ColorMap)
            self.__dialogPlot.replot()


    def setImage(self, im):
        for i in range(4):
            self.enableAxis(i, True)
        if self.__image is None:
            self.__image = TrImageItem()#make.image(im)
            self.__image.set_interpolation("nearest")
            if self.__LUTrange is not None:
                #self.__image.set_lut_range(self.__LUTrange)
                self.__image.set_data(im, lut_range=self.__LUTrange)
            else:
                self.__image.set_data(im)
            self.__image.set_color_map(self.__ColorMap)

            self.__dialogImage  = make.image(im, interpolation="nearest")

            self.__dialogImage.set_color_map(self.__ColorMap)
            self.add_item(self.__image)
        else:
            self.__image.set_data(im, lut_range=self.__LUTrange)
            self.__image.set_color_map(self.__ColorMap)
            self.__dialogImage.set_color_map(self.__ColorMap)
            self.__dialogImage.set_data(im, lut_range=self.__LUTrange)
            if self.__dialogPlot is not None:
                self.__dialogPlot.replot()
            self.replot()
        for i in range(0, 4):
            self.enableAxis(i, False)

        self.__imageData = im


class ImageListItem(QStandardItem):
    def __init__(self, name, model, **params):
        super(ImageListItem,self).__init__(name, **params)
        self.__sliceId = None
        self.__image = None
        self.__histRange = (0, 100)
        self.__histPxY = 15
        self.__imageData = None
        self.__histData = None
        self.__colorTable = [qRgb(128 + i, i, 255 - i) for i in range(256)]
        self.__height = None
        self.__imageWidth = None
        self.__cmapHist = None

        self.__histValues = None
        self.__histEdges = None
        self.__histType = "cmap"
        self.__viewModel = model
        self.__name = name

        self.__viewModel.connect(self.__viewModel,
                                 SIGNAL("showHist" + str(self.__name) + "()"),
                                 self.showHist)
        self.__viewModel.connect(self.__viewModel,
                                 SIGNAL("renderImage" + str(self.__name) + "()"),
                                 self.renderImage)

        self.__imageWidget = ImagePlotItem()
        self.__gridParam = make.gridparam(major_enabled=(True, True),
                                          minor_enabled=(False, False))
        self.__histCurvePublic = None
        self.__histCurve = None
        self.setCheckable(True)

        #layout params
        self.__tileRow = None
        self.__tileCol = None
        self.__module = None

    def setLayout(self, tileRow, tileCol, module):
        self.__tileRow = tileRow
        self.__tileCol = tileCol
        self.__module = module

    def getLayout(self):
        return (self.__tileRow, self.__tileCol, self.__module)

    def isOnModule(self, module):
        return self.__module == module

    def isTile(self, row, col):
        return self.__tileRow == row and self.__tileCol == col

    def setSliceId(self, id):
        self.__sliceId = id

    def setTitle(self, title):
        self.setText(str(title))
        self.__imageWidget.set_title(str(title))

    def getSliceId(self):
        return self.__sliceId


    def setHist(self, height, showHist=True):
        im = self.__imageData
        if height is not None:
            self.__height = height


        h, e = np.histogram(im.flatten(), bins=self.__bins,
                            range=self.__histRange)
        self.__histValues = h
        self.__histEdges = (e[:-1]+e[1:])/2.

        self.__histCurve = make.curve(e, h, str(self.__sliceId))
        self.__histCurvePublic = copy.copy(self.__histCurve)

        h = self.__histValues
        s = np.array(h, float)
        a = np.rot90(np.repeat(s, self.__height).reshape(
                                (self.__bins, self.__height)))
        a /= np.max(a)
        a *= 255
        self.__cmapHist = np.array(a, np.uint8).tostring()

        if showHist:
            self.__viewModel.emit(SIGNAL("showHist"+str(self.__name)+"()"))

    def setHistType(self, type):
        self.__histType = type

    def sigShowHist(self):
        self.__viewModel.emit(SIGNAL("showHist"+str(self.__name)+"()"))

    def sigRenderImage(self):
        self.__viewModel.emit(SIGNAL("renderImage"+str(self.__name)+"()"))

    def showHist(self):
        if self.__histType == "cmap":
            image = QImage(self.__cmapHist, self.__bins, self.__height,
                           QImage.Format_Indexed8)
            image.setColorTable(self.__colorTable)

            self.__histData = QPixmap.fromImage(image)
            self.setData(self.__histData, Qt.DecorationRole)
        else:
            plot = CurvePlot(gridparam=self.__gridParam)
            plot.setFixedWidth(self.__bins)
            plot.setFixedHeight(2 * self.__height)
            plot.add_item(self.__histCurve)
            for i in range(4):
                plot.enableAxis(i, False)
            plot.replot()
            self.__histData = QPixmap.grabWidget(plot)
            self.setData(self.__histData, Qt.DecorationRole)

    def getHistCurve(self):
        return self.__histCurvePublic

    def setHistLimits(self, r):
        self.__histRange = r
        if self.__imageData is not None and self.__height is not None:
            self.setHist(None)

    def setLUTLimits(self, r):
        self.__imageWidget.emit(SIGNAL("updateLUTRange(PyQt_PyObject)"), r)

    def updateColorMap(self, map):
        self.__imageWidget.emit(SIGNAL("updateColorMap(PyQt_PyObject)"), map)

    def setHistBins(self, bins):
        self.__bins = bins

    def setWidth(self, widgetWidth):
        if self.__imageData is not None:
            self.__imageWidget.setFixedWidth(widgetWidth)
            self.__imageWidget.setFixedHeight(self._heightForWidth(widgetWidth))
            self.__imageWidget.setMinimumWidth(widgetWidth)
            self.__imageWidget.setMinimumHeight(
                self._heightForWidth(widgetWidth))
        self.__imageWidth = widgetWidth

    def _heightForWidth(self, widgetWidth):
        return (self.__imageData.shape[1] / self.__imageData.shape[0] *
                widgetWidth + 20)

    def prepareImageData(self, value):
        dims = value.get('dims')
        if len(dims) < 2: return
        dimX = dims[0]
        dimY = dims[1]
        dimZ = dims[2]
        encoding = value.get('encoding')
        channelSpace = value.get('channelSpace')
        data = value.get('data')

        imData = None

        if enums[encoding] == "RGBA":
            sliceData = data[int(self.__sliceId * dimX * dimY * 4):
                             int((self.__sliceId + 1) * dimX * dimY * 4)]
            image = QImage(sliceData, dimX, dimY,
                           QImage.Format_ARGB32_Premultiplied)
            dataQw = image.bits().asstring(image.numBytes())
            imData = np.frombuffer(dataQw, np.uint8)
            imData.shape = image.height(), image.bytesPerLine() / 4, 4
        else:
            sliceData = data[int(self.__sliceId * dimX * dimY *
                                _returnRawByteWidth(channelSpace)):
                             int((self.__sliceId + 1) * dimX * dimY *
                                 _returnRawByteWidth(channelSpace))]
            imData = _returnRawArray(sliceData, dimX, dimY, channelSpace)

        self.__imageData = imData

    def renderImage(self):
        self.setWidth(self.__imageWidth)
        self.__imageWidget.setImage(self.__imageData)

    def setImage(self, im):
        self.__imageWidget.setImage(im)

    def getImage(self):
        return self.__imageData

    def getWidget(self):
        return self.__imageWidget

    def setNonLockedLimits(self):
       imData = self.getImage()
       self.setHistLimits((np.min(imData), np.max(imData)))
       self.setLUTLimits((np.min(imData), np.max(imData)))

class BackgroundWorker(object):

    def __init__(self, **params):
        self.__activeWorkers = 0
        self.__workersStarted = False
        self.__workerLock = allocate_lock()
        self.__workQueue = []
        self.__alive = True
        self.__maxWorkers = 10
        self.__supervisor = start_new_thread(self._backgroundWorkSupervisor, ())
        self.__globalCallBacks = []


    def __exit__(self, type, value, traceback):
        while not self.__workersStarted:
            pass
        while self.__activeWorkers > 0:
            pass
        self.__alive = False

    def _backgroundWorker(self, func, args=None, callBack=None):
        self.__workerLock.acquire()
        self.__activeWorkers += 1
        self.__workersStarted = True
        self.__workerLock.release()

        if args is None:
            func()
        else:
            func(args)

        self.__workerLock.acquire()
        self.__activeWorkers -= 1
        self.__workerLock.release()

        if callBack is not None:
            callBack()


    def _backgroundWorkSupervisor(self):
        while self.__alive:
            if self.__workQueue:
                #print "Working"
                workTask = self.__workQueue.pop(0)
                objs = workTask["objs"]
                func = workTask["func"]
                args = workTask["args"]
                callBack = workTask["callBack"]

                while self.__activeWorkers > self.__maxWorkers:
                    #time.sleep(0.1)
                    pass

                for i, obj in enumerate(objs):
                    if isinstance(args, list) and len(args) == len(objs):
                        thisarg = args[i]
                    else:
                        thisarg = args

                    if callBack is not None:
                        start_new_thread(self._backgroundWorker,
                                         (getattr(obj, func), thisarg,
                                          getattr(obj, callBack)))
                    else:
                        start_new_thread(self._backgroundWorker,
                                         (getattr(obj, func), thisarg))
            self.__workerLock.acquire()
            if self.__activeWorkers == 0 and self.__workersStarted == True:
                self.__workersStarted = False
                self.__workerLock.release()
                while self.__globalCallBacks:
                    cb = self.__globalCallBacks.pop(0)
                    try:
                        cb["obj"].emit(SIGNAL(cb["sig"]))
                    except:
                        cb()
                        pass
            else:
                self.__workerLock.release()
            #time.sleep(0.1)

    def startJob(self, objs, func, args=None, callBack=None,
                 globalCallBack=None):
        startTime = time.time()
        if globalCallBack is not None:
            self.__globalCallBacks.append(globalCallBack)
        self.__workQueue.append({"objs": objs, "func": func, "args": args,
                                 "callBack": callBack})


class TileSelectButton(QToolButton):
    def __init__(self, module, tileRow, tileCol, **params):
        super(TileSelectButton, self).__init__(**params)
        self.__module = module
        self.__tileRow = tileRow
        self.__tileCol = tileCol

    def getLayout(self):
        return (self.__tileRow, self.__tileCol, self.__module)


class DisplayImage(DisplayWidget):
    category = "Image"
    alias = "Image Stack View"

    def __init__(self, box, parent):
        self.value = None
        super(DisplayImage, self).__init__(box)

        self.__mainWidget = QWidget(parent)
        self.__mainLayout = QVBoxLayout()
        self.__mainWidget.setLayout(self.__mainLayout)
        self.__BackgroundWorker = BackgroundWorker()

        self.__colPadding = 10
        self.__rowPadding = 20
        self.__minWidth = 500
        self.__minHeight = 500
        self.__cols = 3
        self.__actCols = self.__cols
        self.__minPixelValue = 0
        self.__maxPixelValue = 255
        self.__minPixelValueAuto = 0
        self.__maxPixelValueAuto = 255
        self.__lockLUTs = True
        self.__autoRange = True
        self.__profiles = False
        self.__histType = "cmap"
        self.__aggHistDialog  = None
        self.__lineColors = copy.copy(COLORS)
        #delete white
        del self.__lineColors["w"]
        self.__lineColors = self.__lineColors.values()
        self.__availableLineColors = len(self.__lineColors)


        #selection and column tool bar
        self.__toolBar = QWidget()
        self.__toolBarLayout = QHBoxLayout()
        self.__toolBar.setLayout(self.__toolBarLayout)

        self.__columnSlider = QSlider()
        self.__columnSlider.setTickInterval(2)
        self.__columnSlider.setTickPosition(QSlider.TicksAbove)
        self.__columnSlider.setSliderPosition(self.__cols)
        self.__columnSlider.setRange(1,10)
        self.__columnSlider.setSingleStep(1)
        self.__columnSlider.setOrientation(Qt.Horizontal)
        self.connect(self.__columnSlider, SIGNAL('valueChanged(int)'),
                     self._setNumCols)
        self.__toolBarLayout.addWidget(QLabel("Cols:"))
        self.__toolBarLayout.addWidget(self.__columnSlider)

        #image display toolbar
        self.__imageToolBar = QToolBar()
        #self.__imageToolBarLayout = QHBoxLayout()
        #self.__imageToolBar.setLayout(self.__imageToolBarLayout)


        text = "Toggle between histograms and color bars"
        self.__histTypeButton = QToolButton()
        self.__histTypeButton.setIcon(icons.histHist)
        self.__histTypeButton.setToolTip(text)
        self.__histTypeButton.setStatusTip(text)
        self.connect(self.__histTypeButton, SIGNAL("clicked()"),
                     self._histTypeChange)
        self.__imageToolBar.addWidget(self.__histTypeButton)


        text = "Toggle locking of LUT between images"
        self.__lockLUTCheckButton = QToolButton()
        self.__lockLUTCheckButton.setIcon(icons.lock)
        self.__lockLUTCheckButton.setToolTip(text)
        self.__lockLUTCheckButton.setStatusTip(text)
        self.__lockLUTCheckButton.setCheckable(True)
        self.__lockLUTCheckButton.setChecked(True)
        self.connect(self.__lockLUTCheckButton, SIGNAL("clicked()"),
                     self._lockLUTChange)
        self.__imageToolBar.addWidget(self.__lockLUTCheckButton)

        text = "Toggle auto range computation of images"
        self.__autoRangeCheckButton = QToolButton()
        self.__autoRangeCheckButton.setIcon(icons.entire)
        self.__autoRangeCheckButton.setToolTip(text)
        self.__autoRangeCheckButton.setStatusTip(text)
        self.__autoRangeCheckButton.setCheckable(True)
        self.__autoRangeCheckButton.setChecked(True)
        self.connect(self.__autoRangeCheckButton, SIGNAL("clicked()"),
                     self._autoRangeChange)
        self.__imageToolBar.addWidget(self.__autoRangeCheckButton)

        self.__minRangeBox = QSpinBox()
        #self.__minRangeBox.setValidator(QIntValidator())
        self.__minRangeBox.setAccelerated(True)
        self.__minRangeBox.setKeyboardTracking(False)
        self.connect(self.__minRangeBox, SIGNAL("valueChanged(int)"),
                     self._manualRangeChangeMin)
        self.__imageToolBar.addWidget(self.__minRangeBox)

        self.__rangeSlider = RangeSlider(Qt.Horizontal)
        self.__rangeSlider.setMinimum(0)
        self.__rangeSlider.setMaximum(255)
        self.__rangeSlider.setLow(0)
        self.__rangeSlider.setHigh(255)
        self.__rangeSlider.setTickPosition(QSlider.TicksBelow)
        self.connect(self.__rangeSlider, SIGNAL('sliderReleased(int,int)'),
                     self._manualRangeChange)
        self.__imageToolBar.addWidget(self.__rangeSlider)

        self.__maxRangeBox = QSpinBox()
        #self.__maxRangeBox.setValidator(QIntValidator())
        self.__maxRangeBox.setAccelerated(True)
        self.__maxRangeBox.setKeyboardTracking(False)
        self.connect(self.__maxRangeBox, SIGNAL("valueChanged(int)"),
                     self._manualRangeChangeMax)
        self.__imageToolBar.addWidget(self.__maxRangeBox)

        self.__rangeSlider.setEnabled(False)
        self.__minRangeBox.setEnabled(False)
        self.__maxRangeBox.setEnabled(False)

        text = "Change colormap"
        self.__colorMapSelector = QToolButton()
        self.__colorMapSelector.setIcon(build_icon_from_cmap(get_cmap("jet"),
                                                 width=16, height=16))
        self.__colorMapSelector.setToolTip(text)
        self.__colorMapSelector.setStatusTip(text)
        self.__colorMapSelector.setPopupMode(QToolButton.InstantPopup)
        self.__colorMapSelectorMenu = QMenu()
        for cmapName in get_colormap_list():
            cmap = get_cmap(cmapName)
            icon = build_icon_from_cmap(cmap)
            action = self.__colorMapSelectorMenu.addAction(icon, cmapName)
            action.setEnabled(True)
        self.__colorMapSelector.setMenu(self.__colorMapSelectorMenu)
        self.connect(self.__colorMapSelectorMenu, SIGNAL("triggered(QAction*)"),
                     self._activateCmap)

        self.__imageToolBar.addWidget(self.__colorMapSelector)

        text = "Pop up aggregate histogram window"
        self.__aggHistButton = QToolButton()
        self.__aggHistButton.setIcon(icons.histHist)
        self.__aggHistButton.setToolTip(text)
        self.__aggHistButton.setStatusTip(text)
        self.connect(self.__aggHistButton, SIGNAL("clicked()"),
                     self._createAggHist)
        self.__imageToolBar.addWidget(self.__aggHistButton)

        self.__splitterWidget = QSplitter()
        self.__splitterWidget.setChildrenCollapsible(False)

        self.__leftWidget = QWidget()
        self.__leftLayout = QVBoxLayout()
        self.__leftWidget.setLayout(self.__leftLayout)

        self.__selectControlWidget = QWidget()
        self.__selectControlLayout = QHBoxLayout()
        self.__selectControlWidget.setLayout(self.__selectControlLayout)
        self.__selectBar = QToolBar()
        self.__selectBar.addAction(QAction('A', self,
                                           triggered=self._onSelectAll))
        self.__selectBar.addAction(QAction('N', self,
                                           triggered=self._onSelectNone))
        self.__selectBar.addAction(QAction('I', self,
                                           triggered=self._onSelectInvert))
        self.__selectBar.setToolButtonStyle(Qt.ToolButtonFollowStyle)
        self.__selectControlLayout.addWidget(QLabel("Select:"))
        self.__selectControlLayout.addWidget(self.__selectBar)

        text = "Toggle display of all images or all selected images in selection box"
        self.__CompressButton = QToolButton()
        self.__CompressButton.setText("C")
        self.__CompressButton.setToolTip(text)
        self.__CompressButton.setStatusTip(text)
        self.__CompressButton.setCheckable(True)
        self.__CompressButton.setChecked(False)
        self.connect(self.__CompressButton, SIGNAL("clicked()"),
                     self._toggleCompress)
        self.__selectControlLayout.addWidget(self.__CompressButton)

        self.__listWidget = QListView()
        self.__listWidget.setMinimumWidth(250)


        #self.__listWidget.setSizePolicy(QSizePolicy.Minimum)
        self.__listModel = QStandardItemModel(self.__listWidget)
        self.__listModel.itemChanged.connect(self._onSelectionChanged)


        self.__selectionScrollArea = QScrollArea()
        self.__selectionScrollArea.setWidgetResizable(True)
        self.__selectionScrollArea.setMinimumWidth(self.__minWidth)
        self.__selectionScrollArea.setMinimumHeight(self.__minHeight)


        self.__selectionWidget = QWidget()
        self.__selectionWidget.setMinimumHeight(self.__minHeight)
        self.__selectionWidget.setMinimumWidth(self.__minWidth-20)
        self.__selectionWidget.resizeEvent = self._onResize
        self.connect(self.__selectionWidget, SIGNAL("show()"),
                     self.__selectionWidget.show)

        self.connect(self, SIGNAL("valueChangedCallback()"),
                     self._valueChangedCallback)
        self.connect(self, SIGNAL("updateRangeWidgetsCallback()"),
                     self._updateRangeWidgetsCallback)

        self.__gridLayout = QGridLayout()
        self.__selectionWidget.setLayout(self.__gridLayout)

        self.__selectionScrollArea.setWidget(self.__selectionWidget)

        #tile selection controls
        self.__tileSelectionWidget = QWidget()
        self.__tileSelectionLayout = QGridLayout()
        self.__tileSelectionWidget.setLayout(self.__tileSelectionLayout)


        #selection controls
        self.__selectTileControlWidget = QWidget()
        self.__selectTileControlLayout = QHBoxLayout()
        self.__selectTileControlWidget.setLayout(self.__selectTileControlLayout)
        self.__selectTileBar = QToolBar()
        self.__selectTileBar.addAction(QAction('A', self,
                                       triggered=self._onTileSelectAll))
        self.__selectTileBar.addAction(QAction('N', self,
                                       triggered=self._onTileSelectNone))
        self.__selectTileBar.addAction(QAction('I', self,
                                       triggered=self._onTileSelectInvert))
        self.__selectTileBar.setToolButtonStyle(Qt.ToolButtonFollowStyle)
        self.__selectTileControlLayout.addWidget(QLabel("Tiles:"))
        self.__selectTileControlLayout.addWidget(self.__selectTileBar)

        self.__leftLayout.addWidget(self.__selectControlWidget)
        self.__leftLayout.addWidget(self.__listWidget)
        self.__leftLayout.addWidget(self.__tileSelectionWidget)
        self.__leftLayout.addWidget(self.__selectTileControlWidget)

        self.__splitterWidget.addWidget(self.__leftWidget)
        self.__splitterWidget.addWidget(self.__selectionScrollArea)

        self.__splitterWidget.setStretchFactor(0,0)
        self.__splitterWidget.setStretchFactor(1,1)

        self.__mainLayout.addWidget(self.__toolBar)
        self.__mainLayout.addWidget(self.__imageToolBar)
        self.__mainLayout.addWidget(self.__splitterWidget)
        self.__splitterWidget.setSizes([250,400])


        self.__imageWidth = (self.__splitterWidget.sizes()[1] / self.__cols -
                             self.__colPadding)

        self.__histMin = 0
        self.__histMax = 256
        self.__histBins = 100

        self.__imageLayouts = None
        self.__detectorLayout = None
        self.__tileButtons = []


    @property
    def widget(self):
        return self.__mainWidget


    def _toggleCompress(self):
        self.__selectionWidget.hide()
        if self.__CompressButton.isChecked() is True:
            cnt = 0
            while self.__listModel.item(cnt):
                if not self.__listModel.item(cnt).checkState():
                    self.__listWidget.setRowHidden(cnt, True)
                else:
                    self.__listWidget.setRowHidden(cnt, False)
                cnt += 1
        else:
            cnt = 0
            while self.__listModel.item(cnt):
                self.__listWidget.setRowHidden(cnt, False)
                cnt += 1
        self.__selectionWidget.show()


    def _onTileSelectAll(self):
        for tileButton in self.__tileButtons:
            tileButton.setChecked(True)
        self._onSelectAll()


    def _onTileSelectNone(self):
        for tileButton in self.__tileButtons:
            tileButton.setChecked(False)
        self._onSelectNone()

    def _onTileSelectInvert(self):
        for tileButton in self.__tileButtons:
            if tileButton.isChecked() is True:
                tileButton.setChecked(False)
            else:
                tileButton.setChecked(True)
        self._onSelectInvert()

    def _tileSelectionChange(self):
        self.__listModel.itemChanged.disconnect(self._onSelectionChanged)
        cnt = 0
        while self.__listModel.item(cnt):
            self.__listModel.item(cnt).setCheckState(Qt.Unchecked)
            cnt += 1

        for tileButton in self.__tileButtons:
            if tileButton.isChecked() is True:
                cnt = 0
                while self.__listModel.item(cnt):
                    itemLayout = self.__listModel.item(cnt).getLayout()
                    buttonLayout = tileButton.getLayout()
                    if itemLayout == buttonLayout:
                        self.__listModel.item(cnt).setCheckState(Qt.Checked)
                    cnt += 1
        self.__listModel.itemChanged.connect(self._onSelectionChanged)
        self._onSelectionChanged(None)


    def _updateTileLayoutWidget(self):
        modRows = self.__detectorLayout["moduleRows"]
        modCols = self.__detectorLayout["moduleCols"]
        tileRows = self.__detectorLayout["tileRows"]
        tileCols = self.__detectorLayout["tileCols"]
        self.__tileButtons = []
        m = 0
        for mR in range(0, modRows):
            for mC in range(0, modCols):
                innerTileWidget = QWidget()
                innerTileLayout = QGridLayout()
                innerTileWidget.setLayout(innerTileLayout)
                self.__tileSelectionLayout.addWidget(innerTileWidget, mR, mC,
                                                     1, 1)
                for tR in range(tileRows):
                    for tC in range(tileCols):
                        tileButton = TileSelectButton(m, tR, tC)
                        tileButton.setCheckable(True)
                        tileButton.setChecked(False)
                        self.connect(tileButton, SIGNAL("clicked()"),
                                     self._tileSelectionChange)
                        innerTileLayout.addWidget(tileButton, tR, tC, 1, 1)
                        self.__tileButtons.append(tileButton)
                m += 1

    def _createAggHist(self):
        if self.__aggHistDialog is None:
            self.__aggHistDialog = CurveDialog(edit=True, toolbar=True)
        self._updateAggHist()
        self.__aggHistDialog.show()


    def _updateAggHist(self):
        if self.__aggHistDialog is not None:
            aggHist = self.__aggHistDialog.get_plot()
            items = aggHist.get_items()
            aggHist.del_items(items)
            aggHist.set_titles("Aggregate Histograms of Selected Curves",
                               "Pixel units", "Counts")
            abscnt = 0
            for cnt in range(0,self.__listModel.rowCount()):
                if self.__listModel.item(cnt).checkState():
                    curve = self.__listModel.item(cnt).getHistCurve()
                    #curve.setTitle(str(cnt))
                    pen = QPen(QColor(self.__lineColors[
                            abscnt % self.__availableLineColors]))
                    curve.setPen(pen)
                    curve.curveparam.update_param(curve)
                    aggHist.add_item(curve)
                    abscnt += 1
            legend = make.legend("TR")
            aggHist.add_item(legend)
            aggHist.do_autoscale()


    def _getListModelItems(self):
        items = []
        for slice in range(0,self.__listModel.rowCount()):
            if self.__listModel.item(slice) is not None:
                items.append(self.__listModel.item(slice))
        return items


    def _histTypeChange(self):
        if self.__histType == "cmap":
            self.__histType = "hist"
            self.__histTypeButton.setIcon(icons.histColorMap)
        else:
            self.__histType = "cmap"
            self.__histTypeButton.setIcon(icons.histHist)
        self.__selectionWidget.hide()

        self.__BackgroundWorker.startJob(
            self._getListModelItems(), "setHistType", self.__histType,
            "sigShowHist", {"obj": self.__selectionWidget, "sig": "show()"})


    def _activateCmap(self, action):
        cmapName = str(action.text())
        for slice in range(0,self.__listModel.rowCount()):
            if self.__listModel.item(slice) is not None:
                self.__listModel.item(slice).updateColorMap(cmapName)
        self.__colorMapSelector.setIcon(build_icon_from_cmap_name(cmapName))


    def _updateRangeWidgetsCallback(self):
        self.__minPixelValue = self.__minPixelValueAuto
        self.__maxPixelValue = self.__maxPixelValueAuto
        addRange = 0.1 * np.abs(self.__maxPixelValueAuto -
                                self.__minPixelValueAuto)
        self.__rangeSlider.setMinimum(self.__minPixelValueAuto - addRange)
        self.__rangeSlider.setMaximum(self.__maxPixelValueAuto + addRange)
        self.__rangeSlider.setLow(self.__minPixelValue)
        self.__rangeSlider.setHigh(self.__maxPixelValue)
        self.__rangeSlider.setTickInterval(
            int((self.__rangeSlider.maximum() -
                 self.__rangeSlider.minimum()) / 10))
        self.__minRangeBox.setRange(
            int(self.__minPixelValueAuto - addRange),
            int(self.__maxPixelValueAuto + addRange) - 1)
        self.__maxRangeBox.setRange(
            int(self.__minPixelValueAuto-addRange) + 1,
            int(self.__maxPixelValueAuto+addRange))
        self.__minRangeBox.setValue(int(self.__minPixelValue))
        self.__maxRangeBox.setValue(int(self.__maxPixelValue))
        self.__selectionWidget.show()

        heights = []
        for slice in range(self.__listModel.rowCount()):
            heights.append(self.__listWidget.rectForIndex(
                self.__listModel.indexFromItem(self.__listModel.item(slice))
            ).height() - 2)

        self.__BackgroundWorker.startJob(
            self._getListModelItems(), "setHist", heights, None,
            {"obj": self.__selectionWidget, "sig": "show()"})

    def _valueChangedCallback(self):
        cB = {"obj": self, "sig": "updateRangeWidgetsCallback()"}
        self._setLimits(cB)

    def valueChanged(self, box, value, timestamp=None):
        startTime = time.time()

        if value is None:
            return

        if self.value is None or value is not self.value:
            # Store original value with type
            self.value = value #copy.copy(value)

            if len(value.dims.value) < 3:
                return
            dimX, dimY, dimZ = value.dims.value
            data = value.data.value
            channelSpace = value.channelSpace.value

            if not dimX and not dimY and not dimZ and not data:
                return
            if (dimX < 1 or dimY < 1 or dimZ < 1 or
                len(data) < dimX * dimY * dimZ *
                            _returnRawByteWidth(channelSpace)):
                return


            imageLayouts = self.__imageLayouts
            detectorLayout = self.__detectorLayout
            self.__imageLayouts = None
            self.__detectorLayout = None

            if value.has('header'):
                header = value.get('header')
                if header.has('imageLayouts'):
                    self.__imageLayouts = header.get('imageLayouts')
                if header.has('layout'):
                    self.__detectorLayout = header.get('layout')

            #provide default layout (1 tile on 1 module if none exists)
            if self.__imageLayouts is None:
                imLayouts = []
                for i in range(dimZ):
                    imLayouts.append({"module": 1, "tileRow": 1, "tileCol": 1})
                self.__imageLayouts = imLayouts

            if self.__detectorLayout is None:
                self.__detectorLayout={"moduleRows": 1, "moduleCols": 1,
                                       "tileRows": 1, "tileCols": 1}

            if detectorLayout is not self.__detectorLayout:
                self._updateTileLayoutWidget()

            forceNew = False
            nItems = self.__listModel.rowCount()

            if dimZ != nItems:
                forceNew = True
                #delete previous items in listview
                #print "Deleting previous stack..."
                cnt = 0
                while self.__listModel.item(cnt):
                    self.__listModel.item(cnt).getWidget().hide()
                    self.__listModel.item(cnt).getWidget().setParent(None)
                    cnt += 1
                self.__listModel.clear()

            newModel = self.__listModel

            #hide widget to allow for more reasonable refresh
            self.__selectionWidget.hide()

            if forceNew:
                for slice in range(dimZ):
                    sliceInfo = "({module}-{tileRow}/{tileCol})".format(
                                                **self.__imageLayouts[slice])
                    imageItem = ImageListItem(str(slice), newModel)
                    imageItem.setHistBins(self.__histBins)
                    imageItem.setTitle(str(slice) + sliceInfo)
                    imageItem.setSliceId(slice)
                    imageItem.setWidth(self.__imageWidth)
                    imageItem.setLayout(self.__imageLayouts[slice]["tileRow"],
                                        self.__imageLayouts[slice]["tileCol"],
                                        self.__imageLayouts[slice]["module"])
                    #update grid
                    newModel.appendRow(imageItem)

            items = [newModel.item(i) for i in newModel.rowCount()
                     if newModel.item(i) is not None]

            if forceNew:
                self.__BackgroundWorker.startJob(
                    items, "prepareImageData", value, "sigRenderImage",
                    {"obj": self, "sig": "valueChangedCallback()"})
            else:
                self.__BackgroundWorker.startJob(
                    items, "prepareImageData", value, "sigRenderImage",
                    {"obj": self.__selectionWidget, "sig": "show()"})

            for slice in range(dimZ):
                self.__gridLayout.setRowStretch(slice, 1)
            self.__listModel = newModel
            self.__listWidget.setModel(self.__listModel)


    def _setNumCols(self, cols):
        self.__cols = cols
        self.__imageWidth = (self.__splitterWidget.sizes()[1] / self.__cols -
                             self.__colPadding)
        self._onSelectionChanged(None)


    def _lockLUTChange(self):
        self.__lockLUTs = self.__lockLUTCheckButton.isChecked()
        self._setLimits()

    def _autoRangeChange(self):
        self.__autoRange = self.__autoRangeCheckButton.isChecked()

        if not self.__autoRange:
            self.__lockLUTCheckButton.setChecked(True)
            self.__lockLUTs = True
            self.__rangeSlider.setEnabled(True)
            self.__minRangeBox.setEnabled(True)
            self.__maxRangeBox.setEnabled(True)
            addRange = np.abs(self.__maxPixelValueAuto -
                              self.__minPixelValueAuto) * 0.1
            self.__rangeSlider.setMinimum(self.__minPixelValueAuto-addRange)
            self.__rangeSlider.setMaximum(self.__maxPixelValueAuto+addRange)
            self.__rangeSlider.setLow(self.__minPixelValue)
            self.__rangeSlider.setHigh(self.__maxPixelValue)
            self.__rangeSlider.setTickInterval(
                int((self.__rangeSlider.maximum() -
                     self.__rangeSlider.minimum()) / 10))
            self.__minRangeBox.setRange(
                int(self.__minPixelValueAuto - addRange),
                int(self.__maxPixelValueAuto + addRange) - 1)
            self.__maxRangeBox.setRange(
                int(self.__minPixelValueAuto-addRange) + 1,
                int(self.__maxPixelValueAuto+addRange))
            self.__minRangeBox.setValue(int(self.__minPixelValue))
            self.__maxRangeBox.setValue(int(self.__maxPixelValue))
        else:
            self.__rangeSlider.setEnabled(False)
            self.__minRangeBox.setEnabled(False)
            self.__maxRangeBox.setEnabled(False)
        self._setLimits()

    def _manualRangeChangeMin(self, v):
        try:
            v = int(v)
        except:
            print "Couldnt convert value "+str(v)+" to int"

        csmin = self.__rangeSlider.maximum()

        self.__rangeSlider.setLow(v)
        self.__minPixelValue  = v
        self.__maxRangeBox.setMinimum(int(self.__minPixelValue + 1))
        self._setLimits()

    def _manualRangeChangeMax(self, v):
        try:
            v = int(v)
        except:
            print "Couldnt convert value "+str(v)+" to int"

        csmax = self.__rangeSlider.maximum()

        self.__rangeSlider.setHigh(v)
        self.__maxPixelValue  = v
        self.__minRangeBox.setMaximum(int(self.__minPixelValueAuto - 1))
        self._setLimits()

    def _manualRangeChange(self, v, sliderId):
        if sliderId == 0:
            self.__minPixelValue = v
            self.__minRangeBox.setValue(int(self.__minPixelValue))
            self.__maxRangeBox.setMinimum(int(self.__minPixelValueAuto + 1))
        elif sliderId == 1:
            self.__maxPixelValue  = v
            self.__maxRangeBox.setValue(int(self.__maxPixelValue))
            self.__minRangeBox.setMaximum(int(self.__maxPixelValueAuto) - 1)
        else:
            dV = self.__maxPixelValue-self.__minPixelValue
            self.__maxPixelValue  = v
            self.__minPixelValue = v - dV
            self.__maxRangeBox.setMinimum(int(self.__minPixelValue + 1))
            self.__minRangeBox.setMaximum(int(self.__maxPixelValue - 1))
        self._setLimits()

    def _getAutoRangeLocked(self, value):
        dims = value.get('dims')
        if len(dims) < 2:
            return
        dimX = dims[0]
        dimY = dims[1]
        dimZ = dims[2]
        data = value.get('data')
        encoding = value.get('encoding')
        channelSpace = value.get('channelSpace')
        imData = None
        if encoding == kt.EncodingType.RGBA:
            image = QImage(data, dimX, dimY * dimZ,
                           QImage.Format_ARGB32_Premultiplied)
            dataQw = image.bits().asstring(image.numBytes())
            imData = np.frombuffer(dataQw, np.uint8)
            imData.shape = image.height(), image.bytesPerLine() / 4, 4
        else:
            imData = _returnRawArray(data, dimX, dimY * dimZ, channelSpace)

        self.__minPixelValueAuto = np.min(imData)
        self.__maxPixelValueAuto = np.max(imData)

    def _finalizeLimitSet(self, cB):
        if self.__lockLUTs:
            if self.__autoRange:
                self.__BackgroundWorker.startJob(
                    self._getListModelItems(), "setHistLimits",
                    [self.__minPixelValueAuto, self.__maxPixelValueAuto],
                    None, cB)
                self.__BackgroundWorker.startJob(
                    self._getListModelItems(), "setLUTLimits",
                    [self.__minPixelValueAuto, self.__maxPixelValueAuto],
                    None, cB)
            else:
                self.__BackgroundWorker.startJob(
                    self._getListModelItems(), "setHistLimits",
                    [self.__minPixelValue, self.__maxPixelValue], None, cB)
                self.__BackgroundWorker.startJob(
                    self._getListModelItems(), "setLUTLimits",
                    [self.__minPixelValue, self.__maxPixelValue], None, cB)

    def _setLimits(self, cB=None):
        if cB is None:
            cB = {"obj": self.__selectionWidget, "sig": "show()"}

        self.__selectionWidget.hide()
        if self.__autoRange and self.__lockLUTs:
            self.__BackgroundWorker.startJob(
                [self], "_getAutoRangeLocked", self.value, None,
                lambda: self._finalizeLimitSet(cB))
        elif not self.__autoRange and self.__lockLUTs:
            self._finalizeLimitSet(cB)
        else:
            self.__BackgroundWorker.startJob(
                self._getListModelItems(), "setNonLockedLimits", None, None, cB)
        self._updateAggHist()

    def _onResize(self, event):
        self.__imageWidth = (self.__splitterWidget.sizes()[1] / self.__cols -
                             self.__colPadding)
        cnt = 0
        while self.__listModel.item(cnt):
            self.__listModel.item(cnt).setWidth(self.__imageWidth)
            cnt += 1

        if self.__listModel.item(0):
            self.__selectionWidget.setMinimumHeight(
                (self.__listModel.item(0).getWidget().height() +
                 self.__rowPadding) * self.__gridLayout.rowCount() /
                self.__actCols)

    def _onSelectionChanged(self, item):
        selected = 0
        cnt = 0
        while self.__listModel.item(cnt):
            if self.__listModel.item(cnt).checkState():
                selected += 1
            cnt += 1

        self.__actCols = self.__cols
        if selected < self.__actCols:
            self.__actCols = selected

        for i in reversed(range(self.__gridLayout.count())):
            self.__gridLayout.itemAt(i).widget().hide()
            self.__gridLayout.removeWidget(self.__gridLayout.itemAt(i).widget())

        cnt = 0
        selCnt = 0

        if selected == 0:
            return

        while self.__listModel.item(cnt):
            if self.__listModel.item(cnt).checkState():
                widget = self.__listModel.item(cnt).getWidget()
                widget.show()
                self.__gridLayout.addWidget(
                    widget, np.floor(selCnt / self.__actCols),
                    selCnt % self.__actCols, 1, 1)
                selCnt += 1
            cnt += 1

        #set size of grid widget
        self.__selectionWidget.setMinimumHeight(
            (self.__gridLayout.itemAt(0).widget().height() +
             self.__rowPadding) * self.__gridLayout.rowCount() / self.__actCols)
        self._toggleCompress()
        self._updateAggHist()


    def _onSelectAll(self):
        cnt = 0
        self.__listModel.itemChanged.disconnect(self._onSelectionChanged)
        while self.__listModel.item(cnt):
            self.__listModel.item(cnt).setCheckState(Qt.Checked)
            cnt += 1
        self.__listModel.itemChanged.connect(self._onSelectionChanged)
        self._onSelectionChanged(None)

    def _onSelectNone(self):
        cnt = 0
        self.__listModel.itemChanged.disconnect(self._onSelectionChanged)
        while self.__listModel.item(cnt):
            self.__listModel.item(cnt).setCheckState(Qt.Unchecked)
            cnt += 1
        self.__listModel.itemChanged.connect(self._onSelectionChanged)
        self._onSelectionChanged(None)

    def _onSelectInvert(self):
        cnt = 0
        self.__listModel.itemChanged.disconnect(self._onSelectionChanged)
        while self.__listModel.item(cnt):
            if self.__listModel.item(cnt).checkState():
                self.__listModel.item(cnt).setCheckState(Qt.Unchecked)
            else:
                self.__listModel.item(cnt).setCheckState(Qt.Checked)
            cnt += 1
        self.__listModel.itemChanged.connect(self._onSelectionChanged)
        self._onSelectionChanged(None)
