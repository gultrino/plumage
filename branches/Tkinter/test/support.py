import os
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

def sample_img(name):
    imgs = "sample"
    this_dir = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0]))
    return os.path.join(this_dir, imgs, name)
