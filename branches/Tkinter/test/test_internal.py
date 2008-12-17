import unittest
import threading

import support
import Tkinter

class InternalFuncsTest(unittest.TestCase):

    def setUp(self):
        self.btn = Tkinter.Button()

    def tearDown(self):
        self.btn.destroy()

    def test_cmd_replace(self):
        invoked = []
        def test(): pass
        def test2(): invoked.append(True)

        self.btn['command'] = test
        old_name = self.btn['command']
        self.btn['command'] = test2

        self.failIf(len(self.btn._tclCommands) == 2)
        self.failIf(self.btn.tk.call("info", "commands", old_name))

        self.btn.invoke()
        self.failUnless(invoked)

        cmds_amount = len(self.btn._tclCommands)
        self.failUnlessRaises(Tkinter.TclError, self.btn.configure,
                command_huh=test2)
        self.failIf(cmds_amount != len(self.btn._tclCommands))


def test_main():
    support.run(InternalFuncsTest)

if __name__ == "__main__":
    test_main()
