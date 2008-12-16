import os
import sys
import plumage

# tests in the following modules do require a running GUI
REQUIRE_GUI = []

def show_tcl_tk():
    tk_ver = plumage.TK_VERSION
    print "Tcl %s, Tk %s" % (plumage.TCL_VERSION, tk_ver)

def get_tests(gui_tests=True):
    testdir = os.path.dirname(sys.argv[0]) or os.curdir
    extension = ".py"
    for testname in os.listdir(testdir):
        if testname.startswith('test_') and testname.endswith(extension):
            if not gui_tests and testname in REQUIRE_GUI:
                continue
            yield testname[:-len(extension)]

def run_tests(tests):
    for test in tests:
        module = __import__(test)
        getattr(module, "test_main", None)()

def main(args):
    gui_tests = False
    if 'DISPLAY' in os.environ or (len(args) > 1 and "-g" in args):
        gui_tests = True

    show_tcl_tk()
    run_tests(get_tests(gui_tests))
    if not gui_tests:
        print "\n** GUI tests didn't run **"

if __name__ == "__main__":
    main(sys.argv)
