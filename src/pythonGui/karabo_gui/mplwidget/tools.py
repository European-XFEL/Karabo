from matplotlib.widgets import Cursor


class DataCursor(Cursor):
    def __init__(self, ax, visible, color):
        super(DataCursor, self).__init__(ax, color=color)
        self.text = ax.text(1.0, 1.0, '', horizontalalignment='right',
                            verticalalignment='bottom',
                            transform=ax.transAxes)
        self.visible = visible
        self.text.set_visible(visible)

    def set_visible(self, visible, event):
        self.visible = visible
        self.text.set_visible(visible)
        # when changing visibility, we immediately draw or remove the crosshair
        if visible:
            self.showdata(event)
            super(DataCursor, self).onmove(event)
        else:
            self.clear(event)
        super(DataCursor, self)._update()
        return visible

    def showdata(self, event):
        """print cursor position to the canvas"""
        if not event.inaxes:
            return
        self.text.set_text('x={:.2f}, y={:.2f}'
                           ''.format(event.xdata, event.ydata))

    def clear(self, event):
        """Reimplementation of the Cursor mehtod"""
        # the clear function is triggered by MPL draw_event, we need to
        # reimplement this function because originally if the draw_event is
        # caused by data update, the cursor will be cleared.
        if self.visible:
            return
        # vertical and horizontal lines, defined by base class
        self.linev.set_visible(False)
        self.lineh.set_visible(False)
