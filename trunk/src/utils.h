#ifndef _UTILS_H
#define _UTILS_H

#include "plumage.h"

#define TCL_MAJORMINOR (TCL_MAJOR_VERSION * 100 + TCL_MINOR_VERSION)

/* macro for verifying infinite recursion in a Python object, like a
 * list self contained.
 *
 * - The function using it must have an Py_ssize_t variable named i;
 * - After completing, if i is not 0, then you can prepare for returning NULL;
 * - msg should be a message to be displayed in case infinite recursion is
 *   detected;
 */
#define CheckInfRecursion(msg)                              \
    do {                                                    \
        i = Py_ReprEnter(obj);                              \
        if (i != 0) {                                       \
            if (i > 0)                                      \
                PyErr_SetString(PyExc_RuntimeError, msg);   \
        }                                                   \
    } while (0)

PyObject *TclObj_ToPy(TclInterpObj *, Tcl_Obj *);
Tcl_Obj *PyObj_ToTcl(PyObject *);

#endif
