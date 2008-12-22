import unittest
import threading

import support
import plumage

class BgErrorTest(unittest.TestCase):

    def setUp(self):
        self.interp = plumage.Interp(use_tk=False)

    def test_refcnt(self):
        class MyException(Exception): pass
        def myhandler(err):
            raise MyException(err)

        # Interp was missing a INCREF to bgerror_handler, testing it here just
        # in case..
        interp = plumage.Interp(use_tk=False, bgerror_handler=myhandler)
        del interp

        interp2 = plumage.Interp(use_tk=False, bgerror_handler=myhandler)
        del myhandler

        def invoke_bgerror():
            interp2.call("after", 1, "badbad")
            # XXX explain a double call to do_one_event
            interp2.do_one_event()
            self.failUnlessRaises(MyException, interp2.do_one_event)

        interp2.createcommand("invoke_bgerror", invoke_bgerror)
        interp2.call("after", 1, "invoke_bgerror")
        interp2.do_one_event()



def test_main():
    support.run(BgErrorTest)

if __name__ == "__main__":
    test_main()
