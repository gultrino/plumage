import unittest
import threading

import support
import plumage

class LoadTkTest(unittest.TestCase):

    def setUp(self):
        self.interp = plumage.Interp(use_tk=False)

    def test_loadtk_otherthread(self):
        failed = []
        def loadit():
            self.interp.loadtk()

        t = threading.Thread(target=loadit)
        t.start()
        t.join()

        self.interp.do_one_event()
        self.failUnless(self.interp.tk_loaded,
                "Tk did not load successfully, test failed.")


def test_main():
    support.run(LoadTkTest)

if __name__ == "__main__":
    test_main()
