import os
import sys
import optparse

import plumage

#CHECK_LEAKS = False

def show_tcl_tk():
    tk_ver = plumage.TK_VERSION
    print "Tcl %s, Tk %s" % (plumage.TCL_VERSION, tk_ver)

def get_tests(tests=None):
    testdir = os.path.dirname(sys.argv[0]) or os.curdir
    extension = ".py"
    for testname in os.listdir(testdir):
        if testname.startswith('test_') and testname.endswith(extension):
            name = testname[:-len(extension)]
            if not tests or name in tests:
                yield testname[:-len(extension)]

def run_tests(tests, repeat=1):
    for test in tests:
        module = __import__(test)
        test = getattr(module, "test_main", None)
        for _ in xrange(repeat):
            test()

def opt_check_memleak(option, opt_str, value, parser):
    #if hasattr(sys, 'gettotalrefcount'):
    #    global CHECK_LEAKS
    #    CHECK_LEAKS = True

    if value <= 0:
        raise optparse.OptionValueError("%s option needs a positive "
                "integer, %r received" % (opt_str, value))
    setattr(parser.values, option.dest, value)

def main():
    parser = optparse.OptionParser(usage="%prog [OPTIONS] [TESTS]")

    parser.add_option("-t", "--tests", dest="tests", action="store_true",
            help=("If this option is specified, then only the specified "
                "tests will be executed."))
    parser.add_option("-v", dest="verbosity", action="count",
            help=("Specify the verbose level in which tests will be "
            "executed."))
    parser.add_option("-r", "--repeat", type="int", dest="repeat",
            action="callback", callback=opt_check_memleak,
            help=("Specify an integer value to define how many times each "
                "specified test should run."))# This will also check if the "
                #"reference count increases between executions (if using "
                #"a debug build)"))
    parser.add_option("-g", dest="using_gui", action="store_true",
            help=("Pass this option if DISPLAY is not an environment "
                "variable in your system and still you are running a GUI"))

    opts, args = parser.parse_args()

    tests = None
    if opts.tests:
        tests = args

    repeat = 1
    if opts.repeat:
        repeat = opts.repeat

    gui_tests = False
    if 'DISPLAY' in os.environ or opts.using_gui:
        gui_tests = True
    if not gui_tests:
        raise Exception("All the tests require a running a GUI.")

    show_tcl_tk()
    run_tests(get_tests(tests), repeat)

if __name__ == "__main__":
    main()
