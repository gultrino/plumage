import unittest
import threading

import support
import Tkinter

class TkinterMiscTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()

    def tearDown(self):
        self.root.destroy()

    def test_selection_handle(self):
        def test(): pass

        cmds = self.root._tclCommands.copy()
        self.failUnlessRaises(Tkinter.TclError, self.root.selection_handle,
                test, badoption=True)
        # no command should have been added to _tclCommands
        self.failUnlessEqual(cmds, self.root._tclCommands)

    def test_selection_own(self):
        def test(): pass

        cmds = self.root._tclCommands.copy()
        self.failUnlessRaises(Tkinter.TclError, self.root.selection_own,
                command=test, badoption=True)
        # no command should have been added to _tclCommands
        self.failUnlessEqual(cmds, self.root._tclCommands)


def test_main():
    support.run(TkinterMiscTest)

if __name__ == "__main__":
    test_main()
