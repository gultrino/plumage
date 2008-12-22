import sys
import unittest
import threading

import support
import Tkinter

class AfterCmdTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()

    def tearDown(self):
        self.root.destroy()

    # XXX this may move to another module
    def test_after_leak(self):
        cmd_len = len(self.root._tclCommands)

        try:
            self.root.after("bad", lambda: None)
        except Tkinter.TclError:
            # nothing should have been added to cmds
            pass

        self.failUnlessEqual(cmd_len, len(self.root._tclCommands))


    def test_after(self):
        called = []
        def call():
            called.append(True)
        self.root.after(1, call)
        while not called:
            self.root.do_one_event()
            # if the error is reported by Tk.report_callback_exception it
            # won't raise any exception, but will set sys.last_type and others
            if getattr(sys, 'last_type', None):
                self.fail("'after' failed in some way")


def test_main():
    support.run(AfterCmdTest)

if __name__ == "__main__":
    test_main()
