import unittest
import Tkinter

import support

class TclVariableTest(unittest.TestCase):

    def setUp(self):
        self.var = Tkinter.Variable()

    def tearDown(self):
        del self.var


    def test_var_trace(self):
        def test(x, y, z): pass

        var = Tkinter.Variable()
        master = var._master

        cmds = var._master._tclCommands[:]

        n1 = var.trace("ru", test)
        n2 = var.trace("ru", test)

        # deleting a var should remove any callbacks associated by trace
        self.failIf(cmds == var._master._tclCommands)
        self.failIf(len(var._master._tclCommands) - len(cmds) != 2)
        del var
        self.failIf(cmds != master._tclCommands)

        # check that failing to call trace doesn't leave an unused command
        # around
        cmds = self.var._master._tclCommands[:]
        self.failUnlessRaises(Tkinter.TclError, self.var.trace, "wrong", test)
        self.failIf(cmds != self.var._master._tclCommands)



def test_main():
    support.run(TclVariableTest)

if __name__ == "__main__":
    test_main()
