import unittest
import threading

import support
import Tkinter

class CanvasTest(unittest.TestCase):

    def setUp(self):
        self.canvas = Tkinter.Canvas()

    def tearDown(self):
        self.canvas.destroy()

    def test_coords(self):
        # test that coords accepts both a coordlist ando also x, y, ..
        tag = self.canvas.create_arc(0, 0, 0, 0)
        self.canvas.coords(tag, 5, 5, 10, 10)
        ccoords = self.canvas.coords(tag)

        self.canvas.coords(tag, (5, 5, 10, 10))
        self.failUnlessEqual(ccoords, self.canvas.coords(tag))
        self.canvas.coords(tag, [5, 5, 10, 10])
        self.failUnlessEqual(ccoords, self.canvas.coords(tag))

    def test_item_creation(self):
        # basic create test (actually testing only if Canvas._create works)
        self.failUnlessRaises(Tkinter.TclError, self.canvas.create_arc, 0, 0)
        self.failUnlessRaises(Tkinter.TclError, self.canvas.create_arc)

        self.canvas.create_arc(0, 0, 50, 50, dash=(2, 4))


def test_main():
    support.run(CanvasTest)

if __name__ == "__main__":
    test_main()
