import os
import unittest
import threading

import support
import plumage

class EvalTest(unittest.TestCase):

    def setUp(self):
        self.interp = plumage.Interp(use_tk=False)

    def test_basic_obj_conversion(self):
        self.interp.eval('set a 1')
        self.assertEqual(self.interp.eval('set a'), '1')

        self.interp.eval('incr a')
        self.assertEqual(self.interp.eval('set a'), 2)


    def test_exception(self):
        tcleval = self.interp.eval

        # the variable 'a' doesn't exist here yet
        self.assertRaises(plumage.TclError, tcleval, 'set a')

        self.assertRaises(plumage.TclError, tcleval, 'invalid cmds')

        self.assertRaises(plumage.TclError, tcleval, 'package require DNE')

        # test that eval doesn't override a Python exception with an empty
        # Tcl error
        def err():
            theerrorishere
        self.interp.createcommand('errorishere', err)
        self.assertRaises(NameError, tcleval, 'errorishere')

    def test_evalfile(self):
        interp = self.interp

        f2 = "test_eval.tcl"
        fobj = open(f2, "w")
        script = """set a 1
        set b 2
        set c [expr $a + $b]
        """
        fobj.write(script)
        fobj.close()

        interp.eval(open(f2).read())
        os.remove(f2)
        self.assertEqual(interp.eval('set a'), '1')
        self.assertEqual(interp.eval('set b'), '2')
        self.assertEqual(interp.eval('set c'), 3)


def test_main():
    support.run(EvalTest)

if __name__ == "__main__":
    test_main()
