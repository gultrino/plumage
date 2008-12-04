import support
import unittest

import _tclnull_tonull

tests = (
        (b"\xC0\x80", b"\x00"),
        (b"\xC0\x80\xC0\x80", b"\x00\x00"),
        (b"\xC0\200a\xC0\x80", b"\000a\x00"),
        (b"hi\xC0\x80ihabc", b"hi\x00ihabc")
        )

class ConversionTest(unittest.TestCase):
    def test_conversion(self):
        for test in tests:
            self.failUnlessEqual(_tclnull_tonull.test_conversion(*test), None)

def test_main():
    support.run(ConversionTest)

if __name__ == "__main__":
    test_main()
