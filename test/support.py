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

# taken from pyttk
def simulate_mouse_click(widget, x, y):
    """Generate proper events to click at the x, y position (tries to act
    like an X server)."""
    widget.event_generate('<Enter>', x=0, y=0)
    widget.event_generate('<Motion>', x=x, y=y)
    widget.event_generate('<ButtonPress-1>', x=x, y=y)
    widget.event_generate('<ButtonRelease-1>', x=x, y=y)

def sample_img(name):
    imgs = "sample"
    this_dir = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0]))
    return os.path.join(this_dir, imgs, name)
