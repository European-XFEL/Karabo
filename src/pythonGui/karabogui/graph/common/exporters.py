from numpy import ndarray, save
from pyqtgraph import mkColor
from pyqtgraph.exporters import ImageExporter as PgImageExporter

from karabo.native import Timestamp
from karabogui import util


class NumpyExporter(object):
    def __init__(self, data):
        if not isinstance(data, ndarray):
            raise TypeError("Data must be a Numpy array.")
        self._data = data

    def export(self):
        name = "{}_image_data.npy".format(Timestamp().toLocal())
        filename = util.getSaveFileName(caption="Export Data",
                                        filter="Numpy File (*.npy)",
                                        suffix="npy",
                                        selectFile=name)

        if not filename:
            return

        if not filename.endswith('.npy'):
            filename = '{}.npy'.format(filename)

        save(filename, self._data)


class ImageExporter(object):
    def __init__(self, item):
        self._exporter = PgImageExporter(item)
        params = self._exporter.parameters()
        params['antialias'] = False
        params["background"] = mkColor('w')

    def export(self):
        """Exports the plot to an image"""
        # First we export the contents to a QImage
        qimage = self._exporter.export(toBytes=True)
        name = "{}_image.png".format(Timestamp().toLocal())
        filename = util.getSaveFileName(caption="Save Snapshot",
                                        filter="PNG (*.png)",
                                        suffix="png",
                                        selectFile=name)

        if not filename:
            return

        if not filename.endswith('.png'):
            filename = '{}.png'.format(filename)

        qimage.save(filename)
