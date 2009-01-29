import unittest
import Tkinter

import support

class TextTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()
        self.text = Tkinter.Text(self.root)

    def tearDown(self):
        self.root.destroy()


    def test_dump_leak(self):
        text = self.text

        self.failIf(text._tclCommands)
        # dump creates a new tcl command, but it should be deleted before
        # returning
        text.dump('1.0', 'end', all=True)
        self.failIf(text._tclCommands)

        # check the same thing, but now when dump fails
        self.failUnlessRaises(Tkinter.TclError,
                text.dump, '1.0', 'end', badoption=True)
        self.failIf(text._tclCommands)

        # test passing an unregistered command
        def cmd(a, b, c):
            print a, repr(b), c
        text.dump('1.0', 'end', cmd, all=True)
        self.failIf(text._tclCommands)


def test_main():
    support.run(TextTest)

if __name__ == "__main__":
    test_main()
