"""A simple but flexible modal dialog box."""

__all__ = ['SimpleDialog']

from Tkinter import Toplevel, Label, Frame, Message, Button
from Tkconstants import BOTH, LEFT, RIGHT

class SimpleDialog(Toplevel):
    def __init__(self, master=None, text='', buttons=None, default=None,
                 cancel=None,  title=None, **kw):
        Toplevel.__init__(self, master, **kw)
        self.wm_title(title)
        self.wm_iconname(title)

        spacing = {'padx': 6, 'pady': 6}
        self.message = Message(self, text=text, aspect=400)
        self.message.pack(expand=True, fill=BOTH, **spacing)
        self.frame = Frame(self)
        self.frame.pack(side=RIGHT)
        self.num = default
        self.cancel = cancel
        self.default = default
        self.bind('<Return>', self.return_event)

        buttons = buttons or ()
        for indx, text in enumerate(buttons):
            b = Button(self.frame, text=text,
                       command=(lambda self=self, indx=indx: self.done(indx)))
            b.pack(side=RIGHT, **spacing)
            if indx == default:
                b.config(default='active')

        self.wm_protocol('WM_DELETE_WINDOW', self.wm_delete_window)
        self.wm_transient(master)

    def go(self):
        self.wait_visibility()
        self.grab_set()
        self.mainloop()
        self.destroy()
        return self.num

    def return_event(self, event):
        if self.default is None:
            self.bell()
        else:
            self.done(self.default)

    def wm_delete_window(self):
        if self.cancel is None:
            self.bell()
        else:
            self.done(self.cancel)

    def done(self, num):
        self.num = num
        self.quit()


def example():
    from Tkinter import Tk

    def show_dlg():
        d = SimpleDialog(root,
                         text="This is a test dialog.  "
                              "Would this have been an actual dialog, "
                              "the buttons below would have been glowing "
                              "in soft pink light.\n"
                              "Do you believe this ?",
                         buttons=("Yes", "No", "Cancel"),
                         default=0,
                         cancel=2,
                         title="Test Dialog")
        print d.go()

    root = Tk()
    t = Button(root, text='Test', command=show_dlg)
    q = Button(root, text='Quit', command=t.quit)
    t.pack()
    q.pack()
    root.mainloop()

if __name__ == '__main__':
    example()
