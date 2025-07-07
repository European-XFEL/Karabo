# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import os.path as op

from qtpy.QtGui import QIcon


class Icon:
    """A lazy-loading class for QIcons

    QIcons can only be read once a QApplication is created. This
    class assures that icons are only loaded when needed."""
    def __init__(self, name):
        self.name = name
        self._icon = None

    @property
    def icon(self):
        if self._icon is None:
            self._icon = QIcon(op.join(op.dirname(__file__), self.name))
        return self._icon

    def __get__(self, instance, owner):
        return self.icon


def init():
    """This function actually loads the hitherto lazily loaded icons."""
    d = globals()
    for k, v in d.items():
        if isinstance(v, Icon):
            d[k] = v.icon

    global applyGrey
    applyGrey = QIcon(apply.pixmap(32, QIcon.Disabled, QIcon.On))


about = Icon("about.svg")
add = Icon("add.svg")
alarmGlobal = Icon("critical.svg")
alarmHigh = Icon("critical.svg")
alarmLow = Icon("critical.svg")
alarmVarianceHigh = Icon("critical.svg")
alarmVarianceLow = Icon("critical.svg")
alarmWarning = Icon("alarmwarning.svg")
alignBottom = Icon("align-bottom.svg")
alignLeft = Icon("align-left.svg")
alignRight = Icon("align-right.svg")
alignTop = Icon("align-top.svg")
apply = Icon("apply-32x32.png")
applyConflict = Icon("apply-conflict-32x32.png")
arrow = Icon("arrow.svg")
arrowDown = Icon("arrow-down.svg")
arrowLeft = Icon("arrow-left.svg")
arrowRight = Icon("arrow-right.svg")
arrowUp = Icon("arrow-up.svg")
arrowFancyDown = Icon("fancy-arrow-down.svg")
arrowFancyLeft = Icon("fancy-arrow-left.svg")
arrowFancyRight = Icon("fancy-arrow-right.svg")
arrowFancyUp = Icon("fancy-arrow-up.svg")
beamProfile = Icon("beam-profile.svg")
boolean = Icon("type-boolean.png")
bound = Icon("bound-logo.svg")
boundDevelopment = Icon("bound-logo-development.svg")
bringToFront = Icon("bring-to-front1-32x32.png")
camera = Icon("camera.svg")
change = Icon("change.svg")
consoleMenu = Icon("console-menu.svg")
clock = Icon("clock.svg")
close = Icon("close-icon.svg")
close_small = Icon("close-icon-20x20.svg")
crosshair = Icon("crosshair.svg")
cursorArrow = Icon("cursor-arrow-32x32.png")
cpp = Icon("cpp-logo.svg")
cppDevelopment = Icon("cpp-logo-development.svg")
crop = Icon("crop.svg")
data_analysis = Icon("data_analysis.svg")
delete = Icon("edit-remove.svg")
deviceAlive = Icon("device-alive.svg")
deviceClass = Icon("device-class.png")
deviceIncompatible = Icon("device-incompatible.png")
deviceInstance = Icon("device-instance.png")
deviceInstanceError = Icon("device-instance-error.png")
deviceMonitored = Icon("device-monitored.svg")
deviceOffline = Icon("offline-32x32.png")
deviceOfflineNoPlugin = Icon("offline-no-plugin.png")
deviceOfflineNoServer = Icon("offline-no-server.png")
device_dead = Icon("device-dead.svg")
device_error = Icon("device-error.svg")
device_requested = Icon("device-requested.svg")
device_schema = Icon("device-schema.svg")
dock = Icon("down-32x32.png")
download = Icon("download.svg")
downsample = Icon("downsample.svg")
draw = Icon("draw.svg")
drawROI = Icon("draw-roi.svg")
drawCrosshair = Icon("draw-crosshair.svg")
edit = Icon("edit.svg")
editClear = Icon("edit-clear-32x32.png")
editCopy = Icon("edit-copy-32x32.png")
editCut = Icon("edit-cut-32x32.png")
editPaste = Icon("edit-paste.svg")
editPasteReplace = Icon("edit-paste-replace.svg")
entireWindow = Icon("entire.svg")
enum = Icon("type-list.png")
eraser = Icon("eraser.svg")
exit = Icon("exit-32x32.png")
export = Icon("export.svg")
file = Icon("file.svg")
filein = Icon("filein-32x32.png")
fileout = Icon("fileout-32x32.png")
float = Icon("type-float.png")
folder = Icon("folder.svg")
folderDomain = Icon("folder-domain")
folderTrash = Icon("folder-trash.svg")
folderType = Icon("folder-type.svg")
grafana = Icon("grafana.svg")
grid = Icon("grid.svg")
group = Icon("group-32x32.png")
groupGrid = Icon("group-grid.svg")
groupHorizontal = Icon("group-horizontal.svg")
groupVertical = Icon("group-vertical.svg")
halt = Icon("halt.svg")
histogram = Icon("histogram.svg")
host = Icon("computer-32x32.png")
image = Icon("image-gray-32x32.png")
imageMissing = Icon("image-missing.svg")
info = Icon("info.svg")
initconfig = Icon("init-config.svg")
inspect_code = Icon("inspect.svg")
inspect_code_error = Icon("inspect-error.svg")
inspect_code_ok = Icon("inspect-ok.svg")
int = Icon("type-integer.png")
interlock = Icon("interlock.svg")
invisibleEye = Icon("invisibleEye.svg")
kill = Icon("delete-32x32.png")
line = Icon("line.svg")
link = Icon("link-32x32.png")
load = Icon("file-open.svg")
lock = Icon("locked.svg")
logo = Icon("splash.png")
logbook = Icon("electronic-log.svg")
logAlarm = Icon("log-alarm-32x32.png")
logDebug = Icon("log-debug-32x32.png")
logError = Icon("log-error-32x32.png")
logInfo = Icon("log-info-32x32.png")
logMenu = Icon("log-menu.svg")
logWarning = Icon("log-warning-32x32.png")
macro = Icon("macro-logo.svg")
macroDevelopment = Icon("macro-logo-development.svg")
homeEdit = Icon("homeedit.svg")
maximize = Icon("view-maximize.svg")
mediaBackward = Icon("media-reset.svg")
mediaPause = Icon("media-pause.svg")
mediaRecord = Icon("media-record.svg")
mediaStart = Icon("media-start.svg")
mediaStop = Icon("media-stop.svg")
minimize = Icon("view-minimize.svg")
move = Icon("move.svg")
new = Icon("filenew-32x32.png")
no = Icon("no.png")
note_info = Icon("info.svg")
note_warning = Icon("info-help.svg")
note_alert = Icon("info-critical.svg")
off = Icon("off.svg")
on = Icon("on.svg")
path = Icon("type-string.png")
picker = Icon("picker.svg")
plus = Icon("plus.svg")
pointer = Icon("pointer.svg")
popup_sticker = Icon("popup-sticker.svg")
printer = Icon("printer.svg")
propertyMissing = Icon("property-missing.svg")
python = Icon("python-logo.svg")
pythonDevelopment = Icon("python-logo-development.svg")
rect = Icon("rect-32x32.png")
refresh = Icon("refresh-32x32.png")
reset = Icon("reset.svg")
resetTilted = Icon("reset-tilted.svg")
remote = Icon("remote.png")
removeUser = Icon("remove_user.svg")
minus = Icon("minus.svg")
resize = Icon("resize.svg")
revert = Icon("edit-undo.png")
roi = Icon("roi.svg")
run = Icon("run.png")
save = Icon("filesave-32x32.png")
saveAs = Icon("filesaveas-32x32.png")
scatter = Icon("scatter.svg")
scenelink = Icon("scenelink-32x32.png")
selectAll = Icon("select-all.svg")
sendToBack = Icon("bring-to-back1-32x32.png")
settings = Icon("settings.svg")
signal = Icon("type-schema.png")
slot = Icon("type-pair.png")
split = Icon("split.svg")
statusOk = Icon("statusOk.svg")
statusError = Icon("statusError.svg")
statusUnknown = Icon("statusUnknown.svg")
sticker = Icon("sticker.svg")
stop = Icon("stop.svg")
string = Icon("type-string.png")
stringAttribute = Icon("type-string-attribute.png")
switchNormal = Icon("lock_switch.svg")
switchActive = Icon("lock_switch_active.svg")
switchWarning = Icon("lock_switch_warning.svg")
switchCritical = Icon("lock_switch_critical.svg")
target = Icon("target.svg")
text = Icon("text-32x32.png")
transform = Icon("transform-32x32.png")
undefined = Icon("type-undefined.png")
undefinedAttribute = Icon("type-undefined-attribute.png")
undock = Icon("up-32x32.png")
ungroup = Icon("ungroup-32x32.png")
warnGlobal = Icon("warning.svg")
warnHigh = Icon("warning.svg")
warnLow = Icon("warning.svg")
warnVarianceHigh = Icon("warning.svg")
warnVarianceLow = Icon("warning.svg")
weblink = Icon("weblink.svg")
yes = Icon("yes.png")
zoom = Icon("zoom.svg")
zoomImage = Icon("zoom-image.svg")
zoomIn = Icon("zoom-in.svg")
zoomOut = Icon("zoom-out.svg")
