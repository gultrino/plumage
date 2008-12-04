"""
This builds plumage (in a hacky way to not require autotools).

Taken from tkinter-polo repo.
"""
import os
import sys
import subprocess
from distutils.core import setup, Extension

SRCDIR = "src"
TESTDIR = "test"

class MissingTclTkConfig(EnvironmentError):
    pass

def get_paths(path, prefix):
    bash = subprocess.Popen(['bash'], stdin=subprocess.PIPE,
        stdout=subprocess.PIPE)

    bash.stdin.write(bytes("source %s\n" % path, encoding='utf8'))
    for name in ("%s_LIB_SPEC", "%s_INCLUDE_SPEC"):
        bash.stdin.write(bytes("echo `eval echo $%s`\n" % (name % prefix),
            encoding='utf8'))
    result = bash.communicate()[0].strip().split(b'\n')

    if not result[0]:
        raise MissingTclTkConfig("'%s' did not contain a (correct) '%s'" %
            (os.path.dirname(path), "%sConfig.sh" % prefix.lower()))

    libs = result[0].split()
    yield libs[1][2:].decode('utf8')
    yield libs[0][2:].decode('utf8')
    yield result[1][2:].decode('utf8')

def get_tcltk_paths():
    tclconfig = os.environ.get('TCL_CONFIG')
    tkconfig = os.environ.get('TK_CONFIG')

    if not all((tclconfig, tkconfig)):
        raise MissingTclTkConfig("TCL_CONFIG and TK_CONFIG env vars must "
            "exist and should point to tclConfig.sh and tkConfig.sh ")

    paths = zip(get_paths(tclconfig, 'TCL'), get_paths(tkconfig, 'TK'))
    return dict(zip(('libraries', 'library_dirs', 'include_dirs'), paths))


def main(args):
    if not len(args) or args[0].startswith('-'):
        tcltk_paths = {}
    else:
        tcltk_paths = get_tcltk_paths()

    c_tcltk_paths = tcltk_paths
    c_tcltk_paths['include_dirs'] += (SRCDIR, )

    setup(name="plumage",
        ext_modules=[
            Extension("plumage",
                [os.path.join(SRCDIR, "plumage.c"),
                    os.path.join(SRCDIR,  "utils.c")],
                **tcltk_paths),
            # C test
            Extension("_tclnull_tonull",
                [os.path.join(TESTDIR, "_tclnull_tonull.c"),
                    os.path.join(SRCDIR, "utils.c")],
                **c_tcltk_paths),
        ]
    )

if __name__ == "__main__":
    main(sys.argv[1:])
