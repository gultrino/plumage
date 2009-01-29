import unittest
import Tkinter

import support

class WmTest(unittest.TestCase):

    def setUp(self):
        self.root = Tkinter.Tk()

    def tearDown(self):
        self.root.destroy()

    def test_wmprotocol(self):
        test = lambda: None
        cmds = self.root._tclCommands[:]

        self.root.wm_protocol("nicename", test)
        self.root.wm_protocol("nicename", func='')

        self.failUnlessEqual(cmds, self.root._tclCommands)


def test_main():
    support.run(WmTest)

if __name__ == "__main__":
    test_main()
