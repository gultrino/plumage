# Delay import _tkinter until we have set TCL_LIBRARY,
# so that Tcl_FindExecutable has a chance to locate its
# encoding directory.
#
# Unfortunately, we cannot know the TCL_LIBRARY directory
# if we don't know the tcl version, which we cannot find out
# without import Tcl. Fortunately, Tcl will itself look in
# <TCL_LIBRARY>/../tcl<TCL_VERSION>, so anything close to
# the real Tcl library will do.

import os
import sys

def find_path(prefix, basename, env_var):
    for name in os.listdir(prefix):
        if name.startswith(basename):
            pkgdir = os.path.join(prefix, name)
            if os.path.isdir(pkgdir):
                os.environ[env_var] = pkgdir

prefix = os.path.join(sys.prefix, "tcl")
if not os.path.exists(prefix):
    # devdir/../tcltk/lib
    prefix = os.path.join(sys.prefix, os.path.pardir, "tcltk", "lib")
    prefix = os.path.abspath(prefix)
# if this does not exist, no further search is needed
if os.path.exists(prefix):
    if not "TCL_LIBRARY" in os.environ:
        find_path(prefix, 'tcl', 'TCL_LIBRARY')

    # Compute TK_LIBRARY, knowing that it has the same version as Tcl
    import _tkinter
    ver = str(_tkinter.TCL_VERSION)
    if not "TK_LIBRARY" in os.environ:
        v = os.path.join(prefix, 'tk%s' % ver)
        if os.path.exists(os.path.join(v, "tclIndex")):
            os.environ['TK_LIBRARY'] = v

    # We don't know the Tix version, so we must search the entire directory
    if not "TIX_LIBRARY" in os.environ:
        find_path(prefix, 'tix', 'TIX_LIBRARY')
