import os
import sys
import plumage

def show_tcl_tk():
    tk_ver = plumage.TK_VERSION
    print "Tcl %s, Tk %s" % (plumage.TCL_VERSION, tk_ver)

def get_tests(gui_tests=True):
    testdir = os.path.dirname(sys.argv[0]) or os.curdir
    extension = ".py"
    for testname in os.listdir(testdir):
        if testname.startswith('test_') and testname.endswith(extension):
            yield testname[:-len(extension)]

def run_tests(tests):
    for test in tests:
        module = __import__(test)
        getattr(module, "test_main", None)()

def main(args):
    gui_tests = False
    if 'DISPLAY' in os.environ or (len(args) > 1 and "-g" in args):
        gui_tests = True

    if not gui_tests:
        raise Exception("All the tests require a running a GUI.")

    show_tcl_tk()
    run_tests(get_tests(gui_tests))

if __name__ == "__main__":
    main(sys.argv)
