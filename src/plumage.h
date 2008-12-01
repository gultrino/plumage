#ifndef PLUMAGE_H
#define PLUMAGE_H

#include "Python.h"
#include <tcl.h>
#include <tk.h>

typedef struct {
	PyObject_HEAD
	/* type specific fields follow */
	Tcl_Interp *interp;

	Tcl_ThreadId tcl_thread_id;
	long py_thread_id;

	int running;
	int tk_loaded;
	int err_in_cb; /* 1 when error occurs in a callback, 2 when bgerror */
	long err_check_interval;

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
