"""File selection dialog classes."""

__all__ = ['FileDialog', 'LoadFileDialog', 'SaveFileDialog']

import os
import fnmatch

from Tkinter import Toplevel, Frame, PanedWindow
from Tkinter import Button, Entry, Listbox, Scrollbar
from Tkconstants import TOP, BOTTOM, X, Y, BOTH, RIGHT, LEFT, END, E
from Dialog import Dialog

dialogstates = {}

class FileDialog(Toplevel):
    """Standard file selection dialog -- no checks on selected file.

    Usage:

        d = FileDialog(master)
        fname = d.go(dir_or_file, pattern, default, key)
        if fname is None:
            # canceled
            ...
        else:
            # try opening it
            ...

    All arguments to go() are optional.

    The 'key' argument specifies a key in the global dictionary
    'dialogstates', which keeps track of the values for the directory
    and pattern arguments, overriding the values passed in (it does
    not keep track of the default argument!).  If no key is specified,
    the dialog keeps no memory of previous state.  Note that memory is
    kept even when the dialog is canceled.  (All this emulates the
    behavior of the Macintosh file selection dialogs.)
    """

    dlg_title = "File Selection Dialog"

    def __init__(self, master, title=None):
        if title is None:
            title = self.dlg_title

        Toplevel.__init__(self, master)
        self.wm_title(title)
        self.iconname(title)

        self.directory = None

        filterframe = Frame(self)
        self.filter = Entry(filterframe)
        filter_button = Button(filterframe, text="Filter",
                               command=self.filter_command)

        # Middle frame, holds the listboxes
        midframe = Frame(self)
        paned = PanedWindow(midframe, orient='horizontal')
        # Directory listbox
        dirsframe = Frame(paned)
        self.dirs = Listbox(dirsframe, exportselection=0)
        dirsbar = Scrollbar(dirsframe, command=self.dirs.yview)
        self.dirs['yscrollcommand'] = dirsbar.set
        # Files listbox
        filesframe = Frame(paned)
        self.files = Listbox(filesframe, exportselection=False)
        filesbar = Scrollbar(filesframe, command=self.files.yview)
        self.files['yscrollcommand'] = filesbar.set
        # Paned
        paned.add(dirsframe)
        paned.add(filesframe)

        self.selection = Entry(self)
        self.selection.bind('<Return>', self.ok_event)

        # Bottom frame, holds dialog buttons
        btframe = Frame(self)
        ok_button = Button(btframe, text="OK", command=self.ok_command)
        cancel_button = Button(btframe, text="Cancel",
                               command=self.cancel_command)

        # setup bindings
        self.filter.bind('<Return>', self.filter_command)
        self.dirs.bind('<ButtonRelease-1>', self.dirs_select_event)
        self.dirs.bind('<Double-ButtonRelease-1>', self.dirs_double_event)
        self.files.bind('<ButtonRelease-1>', self.files_select_event)
        self.files.bind('<Double-ButtonRelease-1>', self.files_double_event)
        self.wm_protocol('WM_DELETE_WINDOW', self.cancel_command)
        # XXX Are the following okay for a general audience?
        self.bind('<Alt-w>', self.cancel_command)
        self.bind('<Alt-W>', self.cancel_command)

        # pack widgets
        spacing = {'padx': 6, 'pady': 6}

        filterframe.pack(fill=X)
        self.filter.pack(side=LEFT, fill=X, expand=True, **spacing)
        filter_button.pack(side=RIGHT, **spacing)

        midframe.pack(expand=True, fill=BOTH, **spacing)
        paned.pack(expand=True, fill=BOTH)
        self.dirs.pack(side=LEFT, fill=BOTH, expand=True)
        dirsbar.pack(side=LEFT, fill=Y)
        self.files.pack(side=LEFT, fill=BOTH, expand=True)
        filesbar.pack(side=LEFT, fill=Y)

        self.selection.pack(fill=X, **spacing)

        btframe.pack(side=BOTTOM, anchor=E)
        cancel_button.pack(side=LEFT, **spacing)
        ok_button.pack(side=LEFT, **spacing)

    def go(self, dir_or_file=os.curdir, pattern="*", default="", key=None):
        if key and key in dialogstates:
            self.directory, pattern = dialogstates[key]
        else:
            dir_or_file = os.path.expanduser(dir_or_file)
            if os.path.isdir(dir_or_file):
                self.directory = dir_or_file
            else:
                self.directory, default = os.path.split(dir_or_file)
        self.set_filter(self.directory, pattern)
        self.set_selection(default)
        self.filter_command()
        self.selection.focus_set()

        self.result = None
        self.wait_visibility() # window needs to be visible for the grab
        self.grab_set()
        self.mainloop()

        if key:
            directory, pattern = self.get_filter()
            if self.result:
                directory = os.path.dirname(self.result)
            dialogstates[key] = directory, pattern

        self.destroy()
        return self.result

    def close(self, result=None):
        self.result = result
        self.quit()

    def dirs_double_event(self, event):
        self.filter_command()

    def dirs_select_event(self, event):
        sel = self.dirs.curselection()
        if not sel:
            return

        dir, pat = self.get_filter()
        subdir = self.dirs.get(sel[0])
        dir = os.path.normpath(os.path.join(self.directory, subdir))
        self.set_filter(dir, pat)

    def files_double_event(self, event):
        self.ok_command()

    def files_select_event(self, event):
        sel = self.files.curselection()
        if not sel:
            return

        self.set_selection(self.files.get(sel[0]))

    def ok_event(self, event):
        self.ok_command()

    def ok_command(self):
        self.close(self.get_selection())

    def cancel_command(self, event=None):
        self.close()

    def filter_command(self, event=None):
        dir, pat = self.get_filter()
        try:
            names = os.listdir(dir)
        except os.error:
            self.bell()
            return

        self.directory = dir
        self.set_filter(dir, pat)

        names.sort()
        subdirs = [os.pardir]
        matchingfiles = []
        for name in names:
            fullname = os.path.join(dir, name)
            if os.path.isdir(fullname):
                subdirs.append(name)
            elif fnmatch.fnmatch(name, pat):
                matchingfiles.append(name)

        self.dirs.delete(0, END)
        for name in subdirs:
            self.dirs.insert(END, name)

        self.files.delete(0, END)
        for name in matchingfiles:
            self.files.insert(END, name)

        head, tail = os.path.split(self.get_selection())
        if tail == os.curdir:
            tail = ''
        self.set_selection(tail)

    def get_filter(self):
        filter = self.filter.get()
        filter = os.path.expanduser(filter)
        if filter[-1:] == os.sep or os.path.isdir(filter):
            filter = os.path.join(filter, "*")
        return os.path.split(filter)

    def get_selection(self):
        sel = self.selection.get()
        return os.path.expanduser(sel)

    def set_filter(self, dir, pat):
        if not os.path.isabs(dir):
            try:
                pwd = os.getcwd()
            except os.error:
                pwd = None
            else:
                dir = os.path.join(pwd, dir)
                dir = os.path.normpath(dir)
        self.filter.delete(0, END)
        self.filter.insert(END, os.path.join(dir or os.curdir, pat or "*"))

    def set_selection(self, file):
        self.selection.delete(0, END)
        self.selection.insert(END, os.path.join(self.directory, file))


class LoadFileDialog(FileDialog):
    """File selection dialog which checks that the file exists."""

    dlg_title = "Load File Selection Dialog"

    def ok_command(self):
        sel = self.get_selection()
        if not os.path.isfile(sel):
            self.master.bell()
        else:
            self.close(sel)


class SaveFileDialog(FileDialog):
    """File selection dialog which checks that the file may be created."""

    dlg_title = "Save File Selection Dialog"

    def ok_command(self):
        sel = self.get_selection()

        if os.path.exists(sel):
            if os.path.isdir(sel):
                # selection is not a file, keep the FileDialog running
                self.bell()
                return

            dlg = Dialog(self,
                       title="Overwrite Existing File Question",
                       text=("Overwrite existing file %r ?" % sel),
                       bitmap='questhead',
                       default=1,
                       strings=("Yes", "Cancel"))

            if dlg.num == 1:
                # cancelled, keep the FileDialog running
                return

        else:
            head, tail = os.path.split(sel)
            if not os.path.isdir(head):
                # selection is not a file, keep the FileDialog running
                self.bell()
                return

        self.close(sel)


def example():
    """Example program."""
    from Tkinter import Tk
    root = Tk()
    root.withdraw()

    fd = LoadFileDialog(root)
    loadfile = fd.go(key="test")
    fd = SaveFileDialog(root)
    savefile = fd.go(key="test")

    print loadfile, savefile

if __name__ == "__main__":
    example()
