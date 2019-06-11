from numpy import ndarray, save
from pyqtgraph import mkColor
from pyqtgraph.exporters import ImageExporter as PgImageExporter

from karabogui import util


class NumpyExporter(object):
    def __init__(self, data):
        if not isinstance(data, ndarray):
            raise TypeError("Data must be a Numpy array.")
        self._data = data

    def export(self):
        filename = util.getSaveFileName(caption="Export Data",
                                        filter="Numpy File (*.npy)",
                                        suffix="npy",
                                        selectFile="image")

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
        filename = util.getSaveFileName(caption="Save Snapshot",
                                        filter="PNG (*.png)",
                                        suffix="png",
                                        selectFile="image")

        if not filename:
            return

        if not filename.endswith('.png'):
            filename = '{}.png'.format(filename)

        self._exporter.export(filename)
