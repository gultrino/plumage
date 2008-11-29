#include "plumage.h"
#include "utils.h"

/* Convert a Tcl object to a Python object.
 *
 * If NULL is returned something wrong happened and the caller must verify
 * if this occurred in Python or in Tcl. */
PyObject *
TclObj_ToPy(TclInterpObj *self, Tcl_Obj *obj)
{
	PyObject *pyobj = NULL;

	if (obj == NULL)
		return NULL;

	if (obj->typePtr == NULL) {
		int length, i;
		char *objstr = Tcl_GetStringFromObj(obj, &length);

#ifdef Py_USING_UNICODE
		/* If the Tcl object contains any bytes with the top bit set,
		 * it's UTF-8 and we should decode it to Unicode */
		for (i = 0; i < length; i++) {
			if (obj->bytes[i] & 0x80)
				break;
		}

		if (i == length)
			pyobj = PyString_FromStringAndSize(objstr, length);
		else {
			/* Convert UTF-8 to Unicode string */
			pyobj = PyUnicode_DecodeUTF8(objstr, length, "strict");
		}
#else
		pyobj = PyString_FromStringAndSize(objstr, length);
#endif
	}

	else if (obj->typePtr == self->IntType)
		pyobj = PyInt_FromLong(obj->internalRep.longValue);

	else if (obj->typePtr == self->DoubleType)
		pyobj = PyFloat_FromDouble(obj->internalRep.doubleValue);

	else if (obj->typePtr == self->ListType) {
		Py_ssize_t length, i;
		Tcl_Obj *value;
		PyObject *item;

		Tcl_ListObjLength(self->interp, obj, &length);

		/* XXX can't it re-enter here forever ? */
		if (length == 1) {
			printf("single list element!\n");
			Tcl_ListObjIndex(self->interp, obj, 0, &value);
			return TclObj_ToPy(self, value);
		}

		pyobj = PyTuple_New(length);
		if (pyobj == NULL)
			return PyErr_NoMemory();

		for (i = 0; i < length; i++) {
			if (Tcl_ListObjIndex(self->interp, obj, i, &value) != TCL_OK) {
				goto exception;
			} else if (value == NULL) {
				/* index out of range (how ?) */
				break;
			}

			if ((item = TclObj_ToPy(self, value)) == NULL)
				goto exception;
			PyTuple_SET_ITEM(pyobj, i, item);
		}
	}

#if TCL_MAJORMINOR >= 805 /* DictType is new to Tcl 8.5 */
	else if (obj->typePtr == self->DictType) {
		pyobj = PyDict_New();
		if (pyobj == NULL)
			return PyErr_NoMemory();

		Tcl_DictSearch search;
		Tcl_Obj *key, *value;
		PyObject *pykey, *pyval;
		int done;

		if (Tcl_DictObjFirst(self->interp, obj, &search, &key,
					&value, &done) != TCL_OK) {
			goto exception;
		}
		for ( ; !done; Tcl_DictObjNext(&search, &key, &value, &done)) {
			if ((pykey = TclObj_ToPy(self, key)) == NULL)
				goto exception;
			if ((pyval = TclObj_ToPy(self, value)) == NULL) {
				goto exception;
			}
			if (PyDict_SetItem(pyobj, pykey, pyval) == -1)
				goto exception;
		}
		Tcl_DictObjDone(&search);
	}
#endif

	else if (obj->typePtr == self->ByteArrayType) {
		int length;
		unsigned char *bytes = Tcl_GetByteArrayFromObj(obj, &length);
#if PY_VERSION_HEX >= 0x2060000
		pyobj = PyByteArray_FromStringAndSize((const char *)bytes, length);
#else
		pyobj = PyString_FromStringAndSize((const char *)bytes, length);
#endif
	}

	else
		pyobj = PyString_FromString(Tcl_GetStringFromObj(obj, NULL));

	return pyobj;

exception:
	Py_DECREF(pyobj);
	return NULL;
}


/* Convert a Python object to a Tcl object.
 *
 * If NULL is returned something wrong happened and the caller must verify
 * if this occurred in Python or in Tcl. */
#define WARN_NO_TYPE_SUPPORT(obj)										\
	PyString_AsString(PyString_FromFormat(								\
				"'%s' object not supported yet, converting to string",	\
				obj->ob_type->tp_name))
Tcl_Obj *
PyObj_ToTcl(PyObject *obj)
{
	Tcl_Obj *tclobj = NULL;

	if (obj == NULL)
		return NULL;

	if (PyString_Check(obj)) {
		tclobj = Tcl_NewStringObj(PyString_AS_STRING(obj),
				PyString_GET_SIZE(obj));
	}

	else if (PyInt_Check(obj))
		tclobj = Tcl_NewLongObj(PyInt_AS_LONG(obj));

	else if (PyFloat_Check(obj))
		tclobj = Tcl_NewDoubleObj(PyFloat_AS_DOUBLE(obj));

	else if (PyBool_Check(obj))
		tclobj = Tcl_NewBooleanObj(PyObject_IsTrue(obj) ? 1 : 0);

	else if (PyUnicode_Check(obj)) {
		PyObject *utf8str = PyUnicode_AsUTF8String(obj);
		if (utf8str == NULL)
			return NULL;

		tclobj = Tcl_NewStringObj(PyString_AS_STRING(utf8str),
				PyString_GET_SIZE(utf8str));
		Py_DECREF(utf8str);
	}

#if PY_VERSION_HEX >= 0x2060000
	else if (PyByteArray_Check(obj)) {
		tclobj = Tcl_NewByteArrayObj(
				(const unsigned char*)PyByteArray_AS_STRING(obj),
				PyByteArray_GET_SIZE(obj));
	}
#endif

	else if (PyTuple_Check(obj) || PyList_Check(obj)) {
		Py_ssize_t i, objc = PySequence_Size(obj);
		Tcl_Obj **objv = (Tcl_Obj **)ckalloc(objc * sizeof(Tcl_Obj *));
		PyObject *temp;

		CheckInfRecursion("sequence has infinite recursion");
		if (i != 0) {
			ckfree((char *)objv);
			return NULL;
		}

		for (i = 0; i < objc; i++) {
			temp = PySequence_GetItem(obj, i);
			objv[i] = PyObj_ToTcl(temp);
			Py_DECREF(temp);

			if (objv[i] == NULL) {
				ckfree((char *)objv);
				return NULL;
			}

			Tcl_IncrRefCount(objv[i]);
		}
		tclobj = Tcl_NewListObj(objc, objv);
		ckfree((char *)objv);
	}


	else if (PyDict_Check(obj)) {
		Tcl_Obj *tkey, *tvalue;
		PyObject *key, *value;
		Py_ssize_t i, pos = 0;

		CheckInfRecursion("dict has infinite recursion");
		/* i has been set by CheckInfRecursion */
		if (i != 0)
			return NULL;

#if TCL_MAJORMINOR >= 805
		tclobj = Tcl_NewDictObj();
#else
		Py_ssize_t j = 0, objc = PyDict_Size(obj) * 2;
		Tcl_Obj **objv;
		objv = (Tcl_Obj **)ckalloc(objc * sizeof(Tcl_Obj *));
#endif

		while (PyDict_Next(obj, &pos, &key, &value)) {
			tkey = PyObj_ToTcl(key);
			tvalue = PyObj_ToTcl(value);
			if (tkey == NULL || tvalue == NULL) {
#if TCL_MAJORMINOR < 805
				ckfree((char *)objv);
#endif
				return NULL;
			}
#if TCL_MAJORMINOR >= 805
			Tcl_DictObjPut(NULL, tclobj, tkey, tvalue);
#else
			objv[2 * j] = tkey;
			objv[2 * j + 1] = tvalue;
			j += 1;
#endif
		}
#if TCL_MAJORMINOR < 805
		tclobj = Tcl_NewListObj(objc, objv);
		ckfree((char *)objv);
#endif
	}

	/* XXX uhmm.. */
	else if (obj == Py_None)
		tclobj = Tcl_NewStringObj("", -1);

	else {
		/* XXX leaving the warning here for now */
		PyErr_Warn(PyExc_RuntimeWarning, WARN_NO_TYPE_SUPPORT(obj));
		PyObject *temp = PyObject_Str(obj);
		tclobj = Tcl_NewStringObj(PyString_AsString(temp), -1);
		Py_DECREF(temp);
	}

	return tclobj;
}
