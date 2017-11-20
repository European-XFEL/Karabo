from PyQt4.QtGui import QIcon
import os.path


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
            self._icon = QIcon(os.path.join(os.path.dirname(__file__),
                                            self.name))
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


entireWindow = Icon("entire.svg")
cursorArrow = Icon("cursor-arrow-32x32.png")
selectAll = Icon("select-all.svg")
text = Icon("text-32x32.png")
line = Icon("line-32x32.png")
link = Icon("link-32x32.png")
scenelink = Icon("scenelink-32x32.png")
rect = Icon("rect-32x32.png")
group = Icon("group-32x32.png")
groupGrid = Icon("group-grid.svg")
groupVertical = Icon("group-vertical.svg")
groupHorizontal = Icon("group-horizontal.svg")
ungroup = Icon("ungroup-32x32.png")
edit = Icon("edit.svg")
editCut = Icon("edit-cut-32x32.png")
editCopy = Icon("edit-copy-32x32.png")
editPaste = Icon("edit-paste.svg")
editPasteReplace = Icon("edit-paste-replace.svg")
editFind = Icon("edit-find.svg")
bringToFront = Icon("bring-to-front1-32x32.png")
sendToBack = Icon("bring-to-back1-32x32.png")
lock = Icon("locked.svg")
unlock = Icon("unlocked.svg")
remote = Icon("remote.png")
exit = Icon("exit-32x32.png")
delete = Icon("edit-remove.svg")
enum = Icon("type-list.png")
string = Icon("type-string.png")
path = Icon("type-string.png")
entire = Icon("entire.svg")
histHist = Icon("hist-hist.svg")
histColorMap = Icon("hist-color-map.svg")
int = Icon("type-integer.png")
float = Icon("type-float.png")
boolean = Icon("type-boolean.png")
undefined = Icon("type-undefined.png")
apply = Icon("apply-32x32.png")
applyConflict = Icon("apply-conflict-32x32.png")
yes = Icon("yes.png")
no = Icon("no.png")
host = Icon("computer-32x32.png")
deviceClass = Icon("device-class.png")
deviceInstance = Icon("device-instance.png")
deviceMonitored = Icon("device-monitored.svg")
deviceInstanceError = Icon("device-instance-error.png")
deviceOffline = Icon("offline-32x32.png")
deviceOfflineNoServer = Icon("offline-no-server.png")
deviceOfflineNoPlugin = Icon("offline-no-plugin.png")
deviceIncompatible = Icon("device-incompatible.png")
signal = Icon("type-schema.png")
slot = Icon("type-pair.png")
image = Icon("image-gray-32x32.png")
folder = Icon("folder.svg")
folderCloud = Icon("folder-cloud.svg")
folderRecord = Icon("folder-record.svg")
revert = Icon("edit-undo.png")
load = Icon("file-open.svg")
load_from_disk = Icon("file-open-from-disk.svg")
save = Icon("filesave-32x32.png")
saveAs = Icon("filesaveas-32x32.png")
file = Icon("file.svg")
device_dead = Icon("device-dead.svg")
device_requested = Icon("device-requested.svg")
device_schema = Icon("device-schema.svg")
device_error = Icon("device-error.svg")
deviceAlive = Icon("device-alive.svg")
propertyMissing = Icon("property-missing.svg")
start = Icon("run.png")
kill = Icon("delete-32x32.png")
new = Icon("filenew-32x32.png")
transform = Icon("transform-32x32.png")
editClear = Icon("edit-clear-32x32.png")
undock = Icon("up-32x32.png")
dock = Icon("down-32x32.png")
maximize = Icon("view-maximize.svg")
minimize = Icon("view-minimize.svg")
logDebug = Icon("log-debug-32x32.png")
logInfo = Icon("log-info-32x32.png")
logWarning = Icon("log-warning-32x32.png")
logError = Icon("log-error-32x32.png")
logAlarm = Icon("log-alarm-32x32.png")
fileout = Icon("fileout-32x32.png")
filein = Icon("filein-32x32.png")
configure = Icon("configure-32x32.png")
refresh = Icon("refresh-32x32.png")
stop = Icon("stop.png")
transformScaleUp = Icon("transform-scale-up-32x32.png")
transformScaleDown = Icon("transform-scale-down-32x32.png")
trendline = Icon("trendline.svg")
choose = Icon("choose.svg")
tableOnline = Icon("online.png")
tablePending = Icon("wait.gif")
tableOffline = Icon("offline.png")
warnGlobal = Icon("warning.svg")
warnLow = Icon("warning.svg")
warnHigh = Icon("warning.svg")
warnVarianceLow = Icon("warning.svg")
warnVarianceHigh = Icon("warning.svg")
alarmGlobal = Icon("critical.svg")
alarmLow = Icon("critical.svg")
alarmHigh = Icon("critical.svg")
alarmVarianceLow = Icon("critical.svg")
alarmVarianceHigh = Icon("critical.svg")
interlock = Icon("interlock.svg")
zoom = Icon("zoom.svg")
crop = Icon("transform-crop.svg")
arrowLeft = Icon("arrow-left.svg")
arrowRight = Icon("arrow-right.svg")
helpcall = Icon("helpcall.svg")
