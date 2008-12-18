import unittest
import threading

import support
import Tkinter

class PanedwindowTest(unittest.TestCase):

    def setUp(self):
        self.pw = Tkinter.PanedWindow()

    def tearDown(self):
        self.pw.destroy()

    def test_paneconfigure(self):
        lbl = Tkinter.Label(self.pw)
        self.pw.add(lbl)

        self.failUnless(isinstance(
            self.pw.paneconfigure(self.pw.panes()[0]), dict))
        test = self.pw.paneconfigure(self.pw.panes()[0], 'minsize')
        self.failUnless(isinstance(test, dict))
        self.failUnlessEqual(len(test), 1)
        self.failUnlessEqual(test.keys(), ['minsize'])

        self.pw.paneconfigure(self.pw.panes()[0], minsize=15)
        self.failUnlessEqual(self.pw.paneconfigure(self.pw.panes()[0],
            'minsize')['minsize'], ('', '', '0', 15))



def test_main():
    support.run(PanedwindowTest)

if __name__ == "__main__":
    test_main()
