/* Copyright (c) 2008, Guilherme Polo
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* This is a bridge between Python and Tcl */

#include "plumage.h"
#include "utils.h"

#include "pythread.h"
#include "structmember.h"

static PyObject *TclError = NULL;
static PyObject *TkError = NULL;


static PyObject *_call(TclInterpObj *, PyObject *);
static PyObject *_eval(TclInterpObj *, PyObject *);
static PyObject *_loadtk(TclInterpObj *, PyObject *);
static PyObject *_schedule_call(TclInterpObj *, PyObject *,
		int(*)(Tcl_Event *, int));

struct QueuedEvent {
	Tcl_EventProc *event;
	struct Tcl_Event *next_ptr;
	/* specific fields */
	TclInterpObj *self;
	PyObject *args;
};

#define CreateEventProc(name, proc)										\
	int	name(Tcl_Event *event, int flags)								\
	{																	\
		struct QueuedEvent *queue_evt = (struct QueuedEvent *)event;	\
		proc(queue_evt->self, queue_evt->args);							\
		return 1;	/* remove event */									\
	}

#define ScheduleIfNeeded(func, args, to_sched)			\
	if (self->tcl_thread_id == Tcl_GetCurrentThread())	\
		return func(self, args);						\
	else												\
		return _schedule_call(self, args, to_sched)

#define RetryIfNeeded(args, function)						\
	if (PyThread_get_thread_ident() != self->py_thread_id)	\
		return _schedule_call(self, args, function)

CreateEventProc(_Event_call, _call)
CreateEventProc(_Event_eval, _eval)
CreateEventProc(_Event_loadtk, _loadtk)


static PyObject *
_schedule_call(TclInterpObj *self, PyObject *args,
		int(*eventproc)(Tcl_Event *, int))
{
	struct QueuedEvent *queue_evt;

	Py_BEGIN_ALLOW_THREADS
	/* Tcl will dealloc queue_evt for us */
	queue_evt = (struct QueuedEvent *)ckalloc(sizeof(struct QueuedEvent));
	queue_evt->event = eventproc;
	queue_evt->self = self;
	queue_evt->args = args;
	Tcl_ThreadQueueEvent(self->tcl_thread_id, (Tcl_Event *)queue_evt,
			TCL_QUEUE_TAIL); //HEAD);
	Py_END_ALLOW_THREADS

	Tcl_ThreadAlert(self->tcl_thread_id);
	Py_RETURN_NONE; /* XXX return an indication of queued event instead ? */
}


static PyObject *
TclInterp_call(TclInterpObj *self, PyObject *args)
{
	Py_INCREF(args);
	ScheduleIfNeeded(_call, args, _Event_call);
}


static PyObject *
TclInterp_eval(TclInterpObj *self, PyObject *args)
{
	Py_INCREF(args);
	ScheduleIfNeeded(_eval, args, _Event_eval);
}


static PyObject *
TclInterp_loadtk(TclInterpObj *self)
{
	ScheduleIfNeeded(_loadtk, NULL, _Event_loadtk);
}


/* XXX The following *_*var functions could be checking for more NULLs
 * but I got tired and didn't do it. */

#define PLUMAGE_VAR_FLAGS TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG

#define CheckResult														\
	if (result == NULL && !PyErr_Occurred())							\
		/* error occurred in the Tcl interpreter */						\
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp))

static PyObject *
TclInterp_get_var(TclInterpObj *self, PyObject *args)
{
	char *varname;
	PyObject *name, *result = NULL;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "s:get_var", &varname))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	result = TclObj_ToPy(self,
			Tcl_ObjGetVar2(self->interp, tclvar, NULL, PLUMAGE_VAR_FLAGS));

	CheckResult;
	return result;
}


static PyObject *
TclInterp_set_var(TclInterpObj *self, PyObject *args)
{
	char *varname;
	PyObject *name, *varval, *result;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sO:set_var", &varname, &varval))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	result = TclObj_ToPy(self,
			Tcl_ObjSetVar2(self->interp, tclvar, NULL, PyObj_ToTcl(varval),
				PLUMAGE_VAR_FLAGS));

	CheckResult;
	return result;
}


static PyObject *
TclInterp_unset_var(TclInterpObj *self, PyObject *args)
{
	char *varname;

	if (!PyArg_ParseTuple(args, "s:unset_var", &varname))
		return NULL;

	if (Tcl_UnsetVar(self->interp, varname, PLUMAGE_VAR_FLAGS) != TCL_OK) {
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
TclInterp_get_arrayvar(TclInterpObj *self, PyObject *args)
{
	char *varname;
	PyObject *name, *element, *result;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sO:get_arrayvar", &varname, &element))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	result = TclObj_ToPy(self,
			Tcl_ObjGetVar2(self->interp, tclvar, PyObj_ToTcl(element),
				PLUMAGE_VAR_FLAGS));

	CheckResult;
	return result;
}


static PyObject *
TclInterp_set_arrayvar(TclInterpObj *self, PyObject *args)
{
	char *varname;
	PyObject *name, *element, *varval, *result;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sOO:set_arrayvar",
				&varname, &element, &varval))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	result = TclObj_ToPy(self,
			Tcl_ObjSetVar2(self->interp, tclvar, PyObj_ToTcl(element),
				PyObj_ToTcl(varval), PLUMAGE_VAR_FLAGS));

	CheckResult;
	return result;
}


static PyObject *
TclInterp_unset_arrayvar(TclInterpObj *self, PyObject *args)
{
	char *varname, *element;

	if (!PyArg_ParseTuple(args, "ss:unset_arrayvar", &varname, &element))
		return NULL;

	if (Tcl_UnsetVar2(self->interp, varname, element,
				PLUMAGE_VAR_FLAGS) != TCL_OK) {
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
_eval(TclInterpObj *self, PyObject *args)
{
	RetryIfNeeded(NULL, _Event_call);

	int flags = -1;
	char *evalstr;
	PyObject *result = NULL;

	if (!PyArg_ParseTuple(args, "s|i:eval", &evalstr, &flags)) {
		Py_DECREF(args);
		return NULL;
	}

	if (flags == -1)
		flags = TCL_EVAL_DIRECT;

	if (Tcl_EvalEx(self->interp, evalstr, -1, flags) != TCL_OK)
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
	else
		result = TclObj_ToPy(self, Tcl_GetObjResult(self->interp));

	Py_DECREF(args);
	return result;
}


static PyObject *
_loadtk(TclInterpObj *self, PyObject *discard)
{
	RetryIfNeeded(NULL, _Event_loadtk);

	if (!self->tk_loaded && (Tk_Init(self->interp) == TCL_ERROR)) {
		PyErr_SetString(TkError, Tcl_GetStringResult(self->interp));
		return NULL;
	}

	self->tk_loaded = 1;
	Py_RETURN_NONE;
}

static PyObject *
_call(TclInterpObj *self, PyObject *args)
{
	RetryIfNeeded(args, _Event_call);

	Tcl_Obj **objv;
	PyObject *retval = NULL;
	Py_ssize_t i, objc = PyTuple_Size(args);

	if (objc == 0) {
		PyErr_SetString(PyExc_TypeError,
				"call expected at least 1 argument, got 0");
		goto finish;
	}

	objv = (Tcl_Obj **)ckalloc(objc * sizeof(Tcl_Obj *));

	/* XXX Not checking for Py_None in args. Current _tkinter stops processing
	 * args when a None is found (I haven't yet decided what to do about
	 * this). */
	for (i = 0; i < objc; i++) {
		objv[i] = PyObj_ToTcl(PyTuple_GetItem(args, i));
		if (objv[i] == NULL)
			goto tcl_dealloc;

		Tcl_IncrRefCount(objv[i]);
	}

	if (Tcl_EvalObjv(self->interp, objc, objv, TCL_EVAL_GLOBAL) != TCL_OK) {
		if (self->_error_in_cb) {
			self->_error_in_cb = 0;
			/* let NULL be returned, error should be set already */
		}
		else
			PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
	} else {
		Tcl_Obj *tclresult = Tcl_GetObjResult(self->interp);
		Tcl_IncrRefCount(tclresult);
		retval = TclObj_ToPy(self, tclresult);
		Tcl_DecrRefCount(tclresult);

		if (retval == NULL) {
			if (PyErr_Occurred() == NULL) {
				/* Exception occurred in Tcl */
				PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
			}
		}
	}

tcl_dealloc:
	i--;
	for (; i >= 0; i--)
		Tcl_DecrRefCount(objv[i]);
	ckfree((char *)objv);

finish:
	Py_DECREF(args);
	return retval;
}



int
TclPyBridge_bgerr(ClientData clientdata, Tcl_Interp *interp, int argc,
		const char *argv[])
{
	TclInterpObj *self = clientdata;
	const char *error_info = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
	PyGILState_STATE gstate = PyGILState_Ensure();

	self->_error_in_cb = 2;
	if (PyErr_Occurred() != NULL) {
		/* error already set by Python */
		goto end;
	}

	if (self->bgerr_handler != NULL) {
		/* call bgerror handler with the received message */
		PyObject_CallFunction(self->bgerr_handler, "s", error_info);
	} else {
		/* generic handler */
		PyErr_SetString(TclError, error_info);
	}

end:
	PyGILState_Release(gstate);
	return TCL_OK;
}

void
TclPyBridge_bgerr_delete(ClientData clientdata)
{
	TclInterpObj *ob = clientdata;
	Py_XDECREF(ob);
}

struct TclPyBridge {
	PyObject *cb;
	PyObject *cb_args;
	TclInterpObj *pytcl;
};

int
TclPyBridge_proc(ClientData clientdata, Tcl_Interp *interp, int objc,
		Tcl_Obj *const objv[])
{
	struct TclPyBridge *cdata = clientdata;
	Py_ssize_t extra_args_size = PyTuple_Size(cdata->cb_args);
	PyGILState_STATE gstate;
	PyObject *func_args, *temp;
	int i, j;

	/* first arg in objv is the command name, discard it */
	--objc;

	gstate = PyGILState_Ensure();

	/* func_args will be a combination (in this order) of args that were
	 * given to Tcl_EvalObjv plus those in cdata->cb_args that were given by
	 * the one whom created the command. */
	func_args = PyTuple_New(objc + extra_args_size);
	for (i = 0; i < objc; i++) {
		temp = TclObj_ToPy(cdata->pytcl, objv[i + 1]);
		if (temp == NULL)
			goto error;
		PyTuple_SET_ITEM(func_args, i, temp);
	}
	for (j = 0; j < extra_args_size; j++) {
		temp = PyTuple_GetItem(cdata->cb_args, j);
		if (temp == NULL)
			goto error;
		PyTuple_SET_ITEM(func_args, j + i, temp);
	}

	temp = PyObject_CallObject(cdata->cb, func_args);
	if (temp == NULL)
		goto error;
	else {
		Tcl_SetObjResult(cdata->pytcl->interp, PyObj_ToTcl(temp));
		Py_DECREF(func_args);
	}

	PyGILState_Release(gstate);

	return TCL_OK;

error:
	cdata->pytcl->_error_in_cb = 1;
	Py_DECREF(func_args);
	PyGILState_Release(gstate);
	return TCL_ERROR;
}

void
TclPyBridge_delete(ClientData clientdata)
{
	struct TclPyBridge *cdata = clientdata;

	if (cdata == NULL) {
		printf("NULL here ? how ?\n");
		return;
	}

	Py_XDECREF(cdata->cb);
	Py_XDECREF(cdata->cb_args);
	Py_XDECREF(cdata->pytcl);
	PyMem_Free(cdata);
}

static PyObject *
TclInterp_createcommand(TclInterpObj *self, PyObject *args)
{
	char *funcname;
	PyObject *cb;
	struct TclPyBridge *clientdata;

	if (!PyArg_ParseTuple(PyTuple_GetSlice(args, 0, 2), "sO:createcommand",
			&funcname, &cb))
		return NULL;

	if (!PyCallable_Check(cb)) {
		PyErr_Format(PyExc_TypeError, "'%s' is not callable",
				cb->ob_type->tp_name);
		return NULL;
	}

	PyObject *cb_args = PyTuple_GetSlice(args, 2, PyTuple_Size(args));

	clientdata = PyMem_Malloc(sizeof(struct TclPyBridge));
	if (clientdata == NULL)
		return PyErr_NoMemory();

	Py_INCREF(cb);
	Py_INCREF(cb_args);
	Py_INCREF(self);
	clientdata->cb = cb;
	clientdata->cb_args = cb_args;
	clientdata->pytcl = self;

	if (Tcl_CreateObjCommand(self->interp, funcname, TclPyBridge_proc,
				clientdata, TclPyBridge_delete) == NULL) {
		PyErr_SetString(TclError, "Tcl interpreter is about to be deleted, "
				"command not created");
		goto error;
	}

	if (PyDict_SetItemString(self->commands, funcname, cb))
		goto error;

	Py_RETURN_NONE;

error:
	TclPyBridge_delete(clientdata);
	return NULL;
}


static PyObject *
TclInterp_deletecommand(TclInterpObj *self, PyObject *args)
{
	char *command;

	if (!PyArg_ParseTuple(args, "s:deletecommand", &command))
		return NULL;

	if (!Tcl_DeleteCommand(self->interp, command)) {
		/* command existed, now it is gone */
		if (PyDict_DelItemString(self->commands, command))
			return NULL;
		Py_RETURN_TRUE;
	} else
		Py_RETURN_FALSE;
}


/* XXX I wish I could just go and change the function below to not use this
 * polling at all. Still need to understand tcl Notifier in order to
 * implement my own or manage to use it in my favor here. */
static PyObject *
TclInterp_mainloop(TclInterpObj *self)
{
#define mainloop_go_on \
	(self->tk_loaded ? self->running && Tk_GetNumMainWindows() : self->running)

	int loop_result = 0;
	self->running = 1;

	/* XXX XXX */
	while (mainloop_go_on) {
		Py_BEGIN_ALLOW_THREADS
		loop_result = Tcl_DoOneEvent(TCL_DONT_WAIT);
		Py_END_ALLOW_THREADS

		if (PyErr_CheckSignals() || self->_error_in_cb == 2)
			break;

		if (loop_result == 0) {
			Py_BEGIN_ALLOW_THREADS
			Tcl_Sleep(20);
			Py_END_ALLOW_THREADS
		}
	}

	self->running = 0;
	self->_error_in_cb = 0;

	if (PyErr_Occurred())
		return NULL;

	Py_RETURN_NONE;
}


static PyObject *
TclInterp_quit(TclInterpObj *self)
{
	self->running = 0;
	Py_RETURN_NONE;
}

static PyMethodDef TclInterp_methods[] = {
	{"call", (PyCFunction)TclInterp_call, METH_VARARGS, NULL},
	{"eval", (PyCFunction)TclInterp_eval, METH_VARARGS, NULL},
	{"load_tk", (PyCFunction)TclInterp_loadtk, METH_NOARGS, NULL},
	{"get_var", (PyCFunction)TclInterp_get_var, METH_VARARGS, NULL},
	{"set_var", (PyCFunction)TclInterp_set_var, METH_VARARGS, NULL},
	{"unset_var", (PyCFunction)TclInterp_unset_var, METH_VARARGS, NULL},
	{"get_arrayvar", (PyCFunction)TclInterp_get_arrayvar, METH_VARARGS, NULL},
	{"set_arrayvar", (PyCFunction)TclInterp_set_arrayvar, METH_VARARGS, NULL},
	{"unset_arrayvar", (PyCFunction)TclInterp_unset_arrayvar, METH_VARARGS,
		NULL},
	{"createcommand", (PyCFunction)TclInterp_createcommand, METH_VARARGS,
		NULL},
	{"deletecommand", (PyCFunction)TclInterp_deletecommand, METH_VARARGS,
		NULL},
	{"mainloop", (PyCFunction)TclInterp_mainloop, METH_NOARGS, NULL},
	{"quit", (PyCFunction)TclInterp_quit, METH_NOARGS, NULL},
	{NULL}
};


void _get_tcltypeobjs(TclInterpObj *self)
{
	self->IntType = Tcl_GetObjType("int");
	self->ListType = Tcl_GetObjType("list");
	self->DictType = Tcl_GetObjType("dict");
	self->DoubleType = Tcl_GetObjType("double");
	self->ByteArrayType = Tcl_GetObjType("bytearray");
}

static PyObject *
TclInterp_New(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	TclInterpObj *self;

	if ((self = (TclInterpObj *)type->tp_alloc(type, 0)) == NULL)
		return NULL;

	self->interp = Tcl_CreateInterp();

	if (Tcl_Init(self->interp) == TCL_ERROR) {
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
		return NULL;
	}

#ifdef TCL_MEM_DEBUG
	Tcl_InitMemory(self->interp);
#endif

	self->py_thread_id = PyThread_get_thread_ident();
	if (Tcl_GetVar2(self->interp,
				"tcl_platform", "threaded", TCL_GLOBAL_ONLY)) {
		self->tcl_thread_id = Tcl_GetCurrentThread();
	}

	if ((self->commands = PyDict_New()) == NULL)
		return NULL;
	self->running = 0;
	self->tk_loaded = 0;
	self->_error_in_cb = 0;

	_get_tcltypeobjs(self);

	/* exit terminates the process (just like the exit in Python), but
	 * why would you want to do that from Tcl besides for annoying me ? */
	Tcl_DeleteCommand(self->interp, "exit");

	Py_INCREF(self);
	Tcl_CreateCommand(self->interp, "bgerror", TclPyBridge_bgerr,
			self, TclPyBridge_bgerr_delete);

	return (PyObject *)self;
}

static int
TclInterp_Init(TclInterpObj *self, PyObject *args, PyObject *kwargs)
{
	PyObject *bgerr_handler = NULL;
	int use_tk = 1;

	/* XXX several args missing here: display, name, use, etc..
	 * will add support for them later */
	static char *kwlist[] = {"use_tk", "bgerror_handler", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", kwlist,
				&use_tk, &bgerr_handler))
		return -1;

	if (use_tk) {
		if (TclInterp_loadtk(self) == NULL)
			return -1;
	}

	if (bgerr_handler != NULL) {
		if (!PyCallable_Check(bgerr_handler)) {
			PyErr_Format(PyExc_TypeError, "bgerror_handler must be a "
					"callable object, '%s' is not",
					bgerr_handler->ob_type->tp_name);
			return -1;
		}
		self->bgerr_handler = bgerr_handler;
	}

	return 0;
}

void
TclInterp_dealloc(TclInterpObj *self)
{
	PyObject *cmdkeys = PyDict_Keys(self->commands);
	Py_ssize_t i, len = PyList_GET_SIZE(cmdkeys);

	for (i = 0; i < len; i++)
		TclInterp_deletecommand(self, PyList_GET_ITEM(cmdkeys, i));

	Py_DECREF(self->commands);
	Py_XDECREF(self->bgerr_handler);
	Tcl_DeleteInterp(self->interp);
	self->ob_type->tp_free((PyObject *)self);
}


static PyObject *
TclInterp_getattr(TclInterpObj *self, char *attr)
{
	PyObject *retval;

	if (!strcmp(attr, "threaded")) {
		retval = PyBool_FromLong(self->tcl_thread_id != NULL);
	} else if (!strcmp(attr, "thread_id")) {
		/* XXX this wasn't updated for py_thread_id */
		if (self->tcl_thread_id == NULL) {
			Py_INCREF(Py_None);
			retval = Py_None;
		} else {
			/* XXX how incorrect is this ? very probably */
			retval = Py_BuildValue("l", self->tcl_thread_id);
		}
	} else if (!strcmp(attr, "tk_loaded")) {
		retval = PyBool_FromLong(self->tk_loaded);
	} else
		retval = Py_FindMethod(TclInterp_methods, (PyObject *)self, attr);

	return retval;
}


static PyTypeObject TclInterpType = {
	PyObject_HEAD_INIT(NULL)
	0,										/* ob_size */
	"plumage.Interp",						/* tp_name */
	sizeof(TclInterpObj),					/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)TclInterp_dealloc,		    /* tp_dealloc */
	0,										/* tp_print */
	(getattrfunc)TclInterp_getattr,			/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_compare */
	0,										/* tp_repr */
	0,										/* tp_as_number */
	0,										/* tp_as_sequence */
	0,										/* tp_as_mapping */
	0,										/* tp_hash */
	0,										/* tp_call */
	0,										/* tp_str */
	0,										/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,						/* tp_flags */
	"Tcl interpreter bridge",				/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	TclInterp_methods,						/* tp_methods */
	0,										/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	(initproc)TclInterp_Init,				/* tp_init */
	0,										/* tp_alloc */
	TclInterp_New							/* tp_new */
};

static PyMethodDef module_methods[] = {
	{NULL}
};


#define AddConst_ErrChk(func, attr, value)	\
	do {									\
		if (func(m, attr, value) == -1)		\
			return;							\
	} while (0)

#define AddStringConst_ErrChk(value) \
	AddConst_ErrChk(PyModule_AddStringConstant, #value, value)

#define AddIntConst_ErrChk(value) \
	AddConst_ErrChk(PyModule_AddIntConstant, #value, value)

PyMODINIT_FUNC
initplumage(void)
{
	PyObject *m;

	if ((m = Py_InitModule("plumage", module_methods)) == NULL)
		return;

	/* exceptions */
	if (!(TclError = PyErr_NewException("plumage.TclError", NULL, NULL)))
		return;
	if (!(TkError = PyErr_NewException("plumage.TkError", TclError, NULL)))
		return;
	Py_INCREF(TclError);
	Py_INCREF(TkError);
	PyModule_AddObject(m, "TclError", TclError);
	PyModule_AddObject(m, "TkError", TkError);

	/* types */
	if (PyType_Ready(&TclInterpType) == -1)
		return;
	Py_INCREF(&TclInterpType);
	PyModule_AddObject(m, "Interp", (PyObject *)&TclInterpType);

	/* constants */
	AddStringConst_ErrChk(TCL_VERSION);
	AddStringConst_ErrChk(TCL_PATCH_LEVEL);
	AddStringConst_ErrChk(TK_VERSION);
	AddStringConst_ErrChk(TK_PATCH_LEVEL);
	/* DoOneEvent flags */
	AddIntConst_ErrChk(TCL_WINDOW_EVENTS);
	AddIntConst_ErrChk(TCL_FILE_EVENTS);
	AddIntConst_ErrChk(TCL_TIMER_EVENTS);
	AddIntConst_ErrChk(TCL_IDLE_EVENTS);
	AddIntConst_ErrChk(TCL_ALL_EVENTS);
	AddIntConst_ErrChk(TCL_DONT_WAIT);

	/* Taken from tcl doc/source (XXX unmix this):
	 *
	 * Tcl_FindExecutable is needed on some platforms in the implementation
	 * of the load command. It is also returned by the info nameofexecutable
	 * command. Based on the locale, determine the encoding of the operating
	 * system and the default encoding for newly opened files.
	 * Called at process initialization time, and part way through startup,
	 * we verify that the initial encodings were correctly setup. Depending
	 * on Tcl's environment, there may not have been enough information first
	 * time through (above).
	 *
	 * The Tcl library path is converted from native encoding to UTF-8, on
	 * the first call, and the encodings may be changed on first or second
	 * call.
	 */
	Tcl_FindExecutable(Py_GetProgramName());
}
