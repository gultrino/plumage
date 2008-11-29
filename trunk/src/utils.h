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

#if 0
typedef struct {
	PyObject_HEAD
	/* type specific fields follow */
	Tcl_Interp *interp;

	Tcl_ThreadId tcl_thread_id;
	long py_thread_id;

	int running;
	int tk_loaded;
	int _error_in_cb; /* 1 when error occurs in a callback, 2 when bgerror */

	PyObject *bgerr_handler;
	PyObject *commands;
	/* Tcl types */
	Tcl_ObjType *IntType;
	Tcl_ObjType *ListType;
	Tcl_ObjType *DictType;
	Tcl_ObjType *DoubleType;
	Tcl_ObjType *ByteArrayType;
} TclInterpObj;
#endif

PyObject *TclObj_ToPy(TclInterpObj *, Tcl_Obj *);
Tcl_Obj *PyObj_ToTcl(PyObject *);

#endif
