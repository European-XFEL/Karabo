import os.path as op

from PyQt4.QtGui import QIcon


class Icon(object):
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


alarmGlobal = Icon("critical.svg")
alarmHigh = Icon("critical.svg")
alarmLow = Icon("critical.svg")
alarmVarianceHigh = Icon("critical.svg")
alarmVarianceLow = Icon("critical.svg")
alarmWarning = Icon("alarmwarning.svg")
apply = Icon("apply-32x32.png")
applyConflict = Icon("apply-conflict-32x32.png")
arrowDown = Icon("arrow-down.svg")
arrowLeft = Icon("arrow-left.svg")
arrowRight = Icon("arrow-right.svg")
arrowUp = Icon("arrow-up.svg")
arrowWheelDown = Icon("arrow-wheel-down.svg")
arrowWheelUp = Icon("arrow-wheel-up.svg")
beamProfile = Icon("beam-profile.svg")
boolean = Icon("type-boolean.png")
booleanAttribute = Icon("type-boolean-attribute.png")
bringToFront = Icon("bring-to-front1-32x32.png")
change = Icon("change.svg")
collapse = Icon("collapse.svg")
consoleMenu = Icon("console-menu.svg")
crossSection = Icon("cross-section.svg")
cursorArrow = Icon("cursor-arrow-32x32.png")
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
edit = Icon("edit.svg")
editClear = Icon("edit-clear-32x32.png")
editCopy = Icon("edit-copy-32x32.png")
editCut = Icon("edit-cut-32x32.png")
editPaste = Icon("edit-paste.svg")
editPasteReplace = Icon("edit-paste-replace.svg")
entireWindow = Icon("entire.svg")
enum = Icon("type-list.png")
enumAttribute = Icon("type-list-attribute.png")
exit = Icon("exit-32x32.png")
expand = Icon("expand.svg")
file = Icon("file.svg")
filein = Icon("filein-32x32.png")
fileout = Icon("fileout-32x32.png")
float = Icon("type-float.png")
floatAttribute = Icon("type-float-attribute.png")
folder = Icon("folder.svg")
folderDomain = Icon("folder-domain")
folderTrash = Icon("folder-trash.svg")
folderType = Icon("folder-type")
group = Icon("group-32x32.png")
groupGrid = Icon("group-grid.svg")
groupHorizontal = Icon("group-horizontal.svg")
groupVertical = Icon("group-vertical.svg")
host = Icon("computer-32x32.png")
image = Icon("image-gray-32x32.png")
int = Icon("type-integer.png")
intAttribute = Icon("type-integer-attribute.png")
interlock = Icon("interlock.svg")
kill = Icon("delete-32x32.png")
line = Icon("line-32x32.png")
link = Icon("link-32x32.png")
load = Icon("file-open.svg")
lock = Icon("locked.svg")
logAlarm = Icon("log-alarm-32x32.png")
logDebug = Icon("log-debug-32x32.png")
logError = Icon("log-error-32x32.png")
logInfo = Icon("log-info-32x32.png")
logMenu = Icon("log-menu.svg")
logWarning = Icon("log-warning-32x32.png")
maximize = Icon("view-maximize.svg")
minimize = Icon("view-minimize.svg")
move = Icon("move.svg")
new = Icon("filenew-32x32.png")
no = Icon("no.png")
path = Icon("type-string.png")
picker = Icon("picker.svg")
pointer = Icon("pointer.svg")
printer = Icon("printer.svg")
propertyMissing = Icon("property-missing.svg")
rect = Icon("rect-32x32.png")
refresh = Icon("refresh-32x32.png")
remote = Icon("remote.png")
resize = Icon("resize.svg")
revert = Icon("edit-undo.png")
roi = Icon("roi.svg")
save = Icon("filesave-32x32.png")
saveAs = Icon("filesaveas-32x32.png")
scenelink = Icon("scenelink-32x32.png")
selectAll = Icon("select-all.svg")
sendToBack = Icon("bring-to-back1-32x32.png")
signal = Icon("type-schema.png")
slot = Icon("type-pair.png")
start = Icon("run.png")
string = Icon("type-string.png")
stringAttribute = Icon("type-string-attribute.png")
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
