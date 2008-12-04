import sys
import unittest

def run(*classes):
    suite = unittest.TestSuite()
    for cls in classes:
        suite.addTest(unittest.makeSuite(cls))

    if '-v' in sys.argv:
        verbosity = 1
    elif '-vv' in sys.argv:
        verbosity = 2
    else:
        verbosity = 0

    runner = unittest.TextTestRunner(sys.stdout, verbosity=verbosity)
    runner.run(suite)
