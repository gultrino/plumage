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

/* XXX I didn't test plumage with anything else besides some tcl/tk 8.4 and
 * some tcl/tk 8.5, but tcl/tk prior to 8.0 will surely not work */
#if TCL_MAJOR_VERSION < 8
#error "Tcl older than 8.0 is not supported"
#endif

static PyObject *TclError = NULL;
static PyObject *TkError = NULL;


static PyObject *TclPyBridge_call(TclInterpObj *, PyObject *);
static PyObject *TclPyBridge_eval(TclInterpObj *, PyObject *);
static PyObject *TclPyBridge_loadtk(TclInterpObj *, PyObject *);
static PyObject *TclPyEvent_schedule_call(TclInterpObj *, PyObject *,
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

#define ScheduleIfNeeded(func, args, to_sched)					\
	if (self->tcl_thread_id == Tcl_GetCurrentThread())			\
		return func(self, args);								\
	else														\
		return TclPyEvent_schedule_call(self, args, to_sched)

#define RetryIfNeeded(args, function)							\
	if (PyThread_get_thread_ident() != self->py_thread_id)		\
		return TclPyEvent_schedule_call(self, args, function)

CreateEventProc(TclPyEvent_call, TclPyBridge_call)
CreateEventProc(TclPyEvent_eval, TclPyBridge_eval)
CreateEventProc(TclPyEvent_loadtk, TclPyBridge_loadtk)


static PyObject *
TclPyEvent_schedule_call(TclInterpObj *self, PyObject *args,
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
	ScheduleIfNeeded(TclPyBridge_call, args, TclPyEvent_call);
}


static PyObject *
TclInterp_eval(TclInterpObj *self, PyObject *args)
{
	Py_INCREF(args);
	ScheduleIfNeeded(TclPyBridge_eval, args, TclPyEvent_eval);
}


static PyObject *
TclInterp_loadtk(TclInterpObj *self)
{
	ScheduleIfNeeded(TclPyBridge_loadtk, NULL, TclPyEvent_loadtk);
}


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

	if (tclvar)
		result = TclObj_ToPy(self,
				Tcl_ObjGetVar2(self->interp, tclvar, NULL, PLUMAGE_VAR_FLAGS));

	CheckResult;
	return result;
}


static PyObject *
TclInterp_set_var(TclInterpObj *self, PyObject *args)
{
	char *varname;
	PyObject *name, *varval, *result = NULL;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sO:set_var", &varname, &varval))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	if (tclvar)
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
	PyObject *name, *element, *result = NULL;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sO:get_arrayvar", &varname, &element))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	if (tclvar)
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
	PyObject *name, *element, *varval, *result = NULL;
	Tcl_Obj *tclvar;

	if (!PyArg_ParseTuple(args, "sOO:set_arrayvar",
				&varname, &element, &varval))
		return NULL;

	name = PyString_FromString(varname);
	tclvar = PyObj_ToTcl(name);
	Py_DECREF(name);

	if (tclvar)
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
TclPyBridge_eval(TclInterpObj *self, PyObject *args)
{
	RetryIfNeeded(NULL, TclPyEvent_eval);

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
TclPyBridge_loadtk(TclInterpObj *self, PyObject *discard)
{
	RetryIfNeeded(NULL, TclPyEvent_loadtk);

	if (!self->tk_loaded && (Tk_Init(self->interp) == TCL_ERROR)) {
		PyErr_SetString(TkError, Tcl_GetStringResult(self->interp));
		return NULL;
	}

	self->tk_loaded = 1;
	Py_RETURN_NONE;
}

static PyObject *
TclPyBridge_call(TclInterpObj *self, PyObject *args)
{
	RetryIfNeeded(args, TclPyEvent_call);

	Tcl_Obj **objv;
	PyObject *tmp, *retval = NULL;
	Py_ssize_t i, objc = PyTuple_Size(args);

	/* XXX Tkinter compatibility:
	 * If args is a single tuple, replace it with the tuple contents */
	if (objc == 1) {
		PyObject *item = PyTuple_GetItem(args, 0);
		if (PyTuple_Check(item)) {
			Py_DECREF(args);
			Py_INCREF(item);
			args = item;
			objc = PyTuple_Size(item);
		}
	}

	if (objc == 0) {
		PyErr_SetString(PyExc_TypeError,
				"call expected at least 1 argument, got 0");
		goto finish;
	}

	objv = (Tcl_Obj **)ckalloc(objc * sizeof(Tcl_Obj *));

	for (i = 0; i < objc; i++) {
		tmp = PyTuple_GetItem(args, i);
		if (tmp == Py_None) {
			/* the argument list stops being processed when a None is found,
			 * this is useful when you are calling a tcl command which may
			 * accept different amount of arguments */
			objc = i;
			break;
		}
		objv[i] = PyObj_ToTcl(tmp);
		if (objv[i] == NULL)
			goto tcl_dealloc;

		Tcl_IncrRefCount(objv[i]);
	}

	if (Tcl_EvalObjv(self->interp, objc, objv, TCL_EVAL_GLOBAL) != TCL_OK) {
		if (self->err_in_cb) {
			self->err_in_cb = 0;
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
		CONST char *argv[])
{
	TclInterpObj *self = clientdata;
	const char *error_info = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
	PyGILState_STATE gstate = PyGILState_Ensure();

	self->err_in_cb = 2;
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
		Tcl_Obj *CONST objv[])
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
	cdata->pytcl->err_in_cb = 1;
	Py_DECREF(func_args);
	PyGILState_Release(gstate);
	return TCL_ERROR;
}

void
TclPyBridge_delete(ClientData clientdata)
{
	struct TclPyBridge *cdata = clientdata;

	Py_XDECREF(cdata->cb);
	Py_XDECREF(cdata->cb_args);
	Py_XDECREF(cdata->pytcl);
	PyMem_Free(cdata);
}

static PyObject *
TclInterp_createcommand(TclInterpObj *self, PyObject *args)
{
	int presult;
	char *funcname;
	PyObject *cb, *name_cb;
	struct TclPyBridge *clientdata;

	name_cb = PyTuple_GetSlice(args, 0, 2);
	presult = PyArg_ParseTuple(name_cb, "sO:createcommand", &funcname, &cb);
	Py_DECREF(name_cb);
	if (!presult)
		return NULL;

	if (!PyCallable_Check(cb)) {
		PyErr_Format(PyExc_TypeError, "'%s' is not callable",
				cb->ob_type->tp_name);
		return NULL;
	}

	PyObject *cb_args = PyTuple_GetSlice(args, 2, PyTuple_Size(args));

	clientdata = PyMem_Malloc(sizeof(struct TclPyBridge));
	if (clientdata == NULL) {
		Py_DECREF(cb_args);
		return PyErr_NoMemory();
	}

	Py_INCREF(cb);
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
		Py_RETURN_TRUE;
	} else
		Py_RETURN_FALSE;
}


void
mainloop_check_signal(ClientData clientdata)
{
	TclInterpObj *self = clientdata;
	PyGILState_STATE gstate = PyGILState_Ensure();

	if (PyErr_CheckSignals() || self->err_in_cb == 2) {
		/* stop the mainloop */
		self->running = 0;
	}
	else
		Tcl_CreateTimerHandler(self->err_check_interval,
				mainloop_check_signal, self);

	PyGILState_Release(gstate);
}

/* XXX I wish I could just go and change the function below to not use this
 * polling at all. Still need to understand tcl Notifier in order to
 * implement my own or manage to use it in my favor here. */
static PyObject *
TclInterp_mainloop(TclInterpObj *self)
{
#define mainloop_go_on \
	(self->tk_loaded ? self->running && Tk_GetNumMainWindows() : self->running)

	Tcl_CreateTimerHandler(self->err_check_interval,
			mainloop_check_signal, self);
	self->running = 1;

	/* XXX XXX */
	while (mainloop_go_on) {
		Py_BEGIN_ALLOW_THREADS
		Tcl_DoOneEvent(TCL_ALL_EVENTS);
		Py_END_ALLOW_THREADS
	}

	self->running = 0;
	self->err_in_cb = 0;

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


static PyObject *
TclInterp_getboolean(TclInterpObj *self, PyObject *args)
{
	int boolval, pytrue;
	PyObject *tclbool, *result = NULL;

	if (!PyArg_ParseTuple(args, "O:getboolean", &tclbool))
		return NULL;

	pytrue = PyObject_IsTrue(tclbool);
	if (pytrue == 1) {
		/* tclbool is considered as True by Python, but maybe this contains
		 * a 'no' string that will be considered as False in Tcl (and that
		 * is what we are checking here). */
		PyObject *o = PyObject_Str(tclbool);
		if (Tcl_GetBoolean(self->interp, PyString_AsString(o),
					&boolval) != TCL_OK)
			PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
		else
			result = PyBool_FromLong(boolval);
		Py_DECREF(o);
	}
	else {
		result = Py_False;
		Py_INCREF(Py_False);
	}

	return result;
}


static PyObject *
TclInterp_splitlist(TclInterpObj *self, PyObject *args)
{
	PyObject *result = NULL;
	char *tcllist;
	const char **elements;
	int listsize, i;

	/* This method is called with the uncertainty of Tcl returning a string
	 * or a Tcl list in some cases. If it happens to return a Tcl list then
	 * it gets converted to a Python tuple and we need to do nothing here. */
	if (PyTuple_Size(args) == 1) {
		result = PyTuple_GetItem(args, 0);
		if (PyTuple_Check(result)) {
			Py_INCREF(result);
			return result;
		}
		result = NULL;
	}

	//if (!PyArg_ParseTuple(args, "et:splitlist", &tcllist))
	if (!PyArg_ParseTuple(args, "s:splitlist", &tcllist))
		return NULL;

	if (Tcl_SplitList(self->interp, tcllist, &listsize, &elements) != TCL_OK)
		PyErr_SetString(TclError, Tcl_GetStringResult(self->interp));
	else {
        result = PyTuple_New(listsize);
        for (i = 0; i < listsize; i++)
            PyTuple_SET_ITEM(result, i, PyString_FromString(elements[i]));

		ckfree((char *)elements);
	}

	return result;
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

	{"getboolean", (PyCFunction)TclInterp_getboolean, METH_VARARGS, NULL},
	{"splitlist", (PyCFunction)TclInterp_splitlist, METH_VARARGS, NULL},

	{NULL}
};


static PyObject *
TclInterp_geterrcheck(TclInterpObj *self, void *closure)
{
	return PyInt_FromLong(self->err_check_interval);
}

int
TclInterp_seterrcheck(TclInterpObj *self, PyObject *value, void *close)
{
	long checkval;

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"Cannot delete err_check_interval attribute");
		return -1;
	}

	if (!PyInt_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"The err_check_interval attribute value must be an int");
		return -1;
	}

	checkval = PyInt_AsLong(value);
	if (checkval < 0) {
		PyErr_SetString(PyExc_TypeError,
				"The err_check_interval attribute value must not be negative");
		return -1;
	}

	self->err_check_interval = checkval;
	return 0;
}

static PyObject *
TclInterp_getthreaded(TclInterpObj *self, void *closure)
{
	return PyBool_FromLong(self->tcl_thread_id != 0);
}

static PyObject *
TclInterp_gettkloaded(TclInterpObj *self, void *closure)
{
	return PyBool_FromLong(self->tk_loaded);
}

static PyObject *
TclInterp_getthreadid(TclInterpObj *self, void *closure)
{
	/* XXX how incorrect is this ? */
	return Py_BuildValue("l", self->tcl_thread_id);
}

static PyGetSetDef TclInterp_getset[] = {
	{"errcheck_interval",
		(getter)TclInterp_geterrcheck, (setter)TclInterp_seterrcheck,
		"Error check interval",
		NULL},
	{"threaded",
		(getter)TclInterp_getthreaded, NULL,
		"Return True if Tcl has thread support, False otherwise",
		NULL},
	{"tk_loaded",
		(getter)TclInterp_gettkloaded, NULL,
		"Return True if Tk has been loaded, False otherwise",
		NULL},
	{"thread_id",
		(getter)TclInterp_getthreadid, NULL,
		"Return the Tcl thread id",
		NULL},
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
		goto error;
	}

#ifdef TCL_MEM_DEBUG
	Tcl_InitMemory(self->interp);
#endif

	self->py_thread_id = PyThread_get_thread_ident();
	self->tcl_thread_id = Tcl_GetCurrentThread();

	self->running = 0;
	self->tk_loaded = 0;
	self->err_in_cb = 0;
	self->err_check_interval = 50;

	_get_tcltypeobjs(self);

	/* exit terminates the process (just like the exit in Python), but
	 * why would you want to do that from Tcl besides for annoying me ? */
	Tcl_DeleteCommand(self->interp, "exit");

	Py_INCREF(self);
	Tcl_CreateCommand(self->interp, "bgerror", TclPyBridge_bgerr,
			self, TclPyBridge_bgerr_delete);

	return (PyObject *)self;

error:
	Py_DECREF(self);
	return NULL;
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
	Py_XDECREF(self->bgerr_handler);
	Tcl_DeleteInterp(self->interp);
	self->ob_type->tp_free((PyObject *)self);
}


static PyTypeObject TclInterpType = {
	PyObject_HEAD_INIT(NULL)
	0,										/* ob_size */
	"plumage.Interp",						/* tp_name */
	sizeof(TclInterpObj),					/* tp_basicsize */
	0,										/* tp_itemsize */
	(destructor)TclInterp_dealloc,		    /* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
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
	TclInterp_getset,						/* tp_getset */
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
