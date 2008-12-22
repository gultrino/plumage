"""
Tkinter interface to the command tk_dialog in tk.
"""

__all__ = ['Dialog', 'DIALOG_ICON']

import Tkinter

DIALOG_ICON = 'questhead'

class Dialog(Tkinter.Toplevel):
    """
    Create modal dialog and wait for response.
    """
    widgetname = 'tk_dialog'

    def __init__(self, master=None, cnf={}, **kw):
        """Construct a tk_dialog with the parent master and the parameters
        in kw.

        kw must contain the following keys:
            title, text, bitmap, default, strings
        """
        Tkinter.Toplevel.__init__(self, master)
        kw.update(cnf)
        args = tuple(kw[arg] for arg in ('title', 'text', 'bitmap', 'default'))
        strings = kw['strings']

        result = self.tk.call(self.widgetname, self._w, *(args + strings))
        if isinstance(result, tuple):
            # We may receive a tuple with a single str
            result = result[0]
        self.num = int(result)

        try:
            self.destroy()
        except Tkinter.TclError:
            pass


def example():
    def test_dialog():
        d = Dialog(
                title='File Modified',
                text=("File 'Python.h' has been modified since the last time "
                      "it was saved. Do you want to save it before exiting "
                      "the application ?"),
                bitmap=DIALOG_ICON,
                strings=('Save File', 'Discard Changes', 'Return to Editor'),
                default=0
                )
        print d.num

    from Tkinter import Button
    test = Button(text='Test', command=test_dialog)
    quit = Button(text='Quit', command=test.quit)
    test.pack()
    quit.pack()
    test.mainloop()

if __name__ == '__main__':
    example()
