import gc
import unittest
import threading

import support
import plumage

class InterpGcTest(unittest.TestCase):

    def test_cyclic(self):
        interp = plumage.Interp(use_tk=False)
        l = [interp]
        interp.x = l
        del interp
        del l

        self.failUnless(gc.collect(), "plumage.Interp not garbage collected")


def test_main():
    support.run(InterpGcTest)

if __name__ == "__main__":
    test_main()
