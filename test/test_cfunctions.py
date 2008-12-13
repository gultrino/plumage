# -*- encoding: utf8 -*-
import unittest

import support
import plumage

class CFunctionsTest(unittest.TestCase):

    def setUp(self):
        self.interp = plumage.Interp(use_tk=False)


    def test_getboolean(self):
        getboolean = self.interp.getboolean

        # some strings that will be evalutated as False by Tcl aswell some
        # Python objects that getboolean should return False
        false = ("no", "false", "off", "0", "", 0, {}, [])
        # some strings that will be evaluated as True by Tcl
        true = ("true", "1", "yes", "on", 1)
        # getboolean should report some TclError on these since they can't
        # be converted to a boolean by Tcl_GetBoolean
        fail = ("[]", "{}", "etc")

        for obj in false:
            self.failUnlessEqual(getboolean(obj), False)

        for obj in true:
            self.failUnlessEqual(getboolean(obj), True)

        for obj in fail:
            self.failUnlessRaises(plumage.TclError, getboolean, obj)


    def test_splitlist(self):
        splitlist = self.interp.splitlist

        # passing a tuple should result in itself being returned
        self.failUnlessEqual(splitlist((1, [2, 3], {4: 5})),
                (1, [2, 3], {4: 5}))

        test = u"á è î õ ü"
        expected = (u"á", u"è", u"î", u"õ", u"ü")
        result = splitlist(test)
        self.failUnlessEqual(result, expected)
        self.failUnlessEqual(len(result), len(expected))

        test = u'\u2603'
        expected = (u'\u2603', )
        result = splitlist(test)
        self.failUnlessEqual(result, expected)
        self.failUnlessEqual(len(result), len(expected))


def test_main():
    support.run(CFunctionsTest)

if __name__ == "__main__":
    test_main()
