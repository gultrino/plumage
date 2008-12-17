import unittest
import threading

import support
import Tkinter

# XXX this may move to another module
class AfterCmdTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()

    def tearDown(self):
        self.root.destroy()

    def test_after_leak(self):
        cmd_len = len(self.root._tclCommands)

        try:
            self.root.after("bad", lambda: None)
        except Tkinter.TclError:
            # nothing should have been added to cmds
            pass

        self.failUnlessEqual(cmd_len, len(self.root._tclCommands))


def test_main():
    support.run(AfterCmdTest)

if __name__ == "__main__":
    test_main()
