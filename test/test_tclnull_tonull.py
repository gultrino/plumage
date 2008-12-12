import unittest

import support
import _tclnull_tonull

tests = (
        ("\xC0\x80", "\x00"),
        ("\xC0\x80\xC0\x80", "\x00\x00"),
        ("\xC0\200a\xC0\x80", "\000a\x00"),
        ("hi\xC0\x80ihabc", "hi\x00ihabc")
        )

class ConversionTest(unittest.TestCase):
    def test_conversion(self):
        for test in tests:
            self.failUnlessEqual(_tclnull_tonull.test_conversion(*test), None)

def test_main():
    support.run(ConversionTest)

if __name__ == "__main__":
    test_main()
