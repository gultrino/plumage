import unittest
import threading

import support
import Tkinter

class MenuTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()
        self.menu = Tkinter.Menu()
        self.root['menu'] = self.menu

    def tearDown(self):
        self.root.destroy()


    def test_menu_add(self):
        def test(): pass

        menu = self.menu

        # Calling into Tcl may cause new commands to be registered, but if the
        # call fails, those commands should be removed. Checking for that here
        cmds = menu._tclCommands.copy()
        self.failUnlessRaises(Tkinter.TclError,
                menu.add_command, command=test, badoption=True)
        self.failUnlessEqual(cmds, menu._tclCommands)

        # testing option parsing
        menu.add_command(command=test, label='test')

        menu.delete(1)
        self.failUnlessEqual(cmds, menu._tclCommands)

    def test_menu_insert(self):
        def test(): pass

        menu = self.menu

        # Calling into Tcl may cause new commands to be registered, but if the
        # call fails, those commands should be removed. Checking for that here
        cmds = menu._tclCommands.copy()
        self.failUnlessRaises(Tkinter.TclError,
                menu.insert_command, 1, command=test, badoption=True)
        self.failUnlessEqual(cmds, menu._tclCommands)

        # testing option parsing
        menu.insert_command(1, command=test, label='test')

        menu.delete(1)
        self.failUnlessEqual(cmds, menu._tclCommands)


def test_main():
    support.run(MenuTest)

if __name__ == "__main__":
    test_main()
