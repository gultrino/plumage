import unittest
import threading

import support
import _utils_bridge

class TypeConversionTest(unittest.TestCase):

    def test_proper_infrec(self):
        # test if Py_ReprEnter/Py_ReprLeave is being used correctly

        test = (1, 2)
        _utils_bridge.test_dummy_pyconversion(test)
        _utils_bridge.test_dummy_pyconversion(test)

        test = {1:2}
        _utils_bridge.test_dummy_pyconversion(test)
        _utils_bridge.test_dummy_pyconversion(test)


def test_main():
    support.run(TypeConversionTest)

if __name__ == "__main__":
    test_main()
