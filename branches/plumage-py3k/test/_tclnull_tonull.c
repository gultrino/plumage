#include "plumage.h"
#include "utils.h"

static TclInterpObj *tclpy;
static PyObject *TestError;


static PyObject *
test_conversion(PyObject *self, PyObject *args)
{
	Py_ssize_t uni_size;
	PyObject *ob, *test;
	int cmpresult;
	Tcl_Obj *obj;

	char *origstr, *expstr;
	int origlen, explen;

	if (!PyArg_ParseTuple(args, "s#s#:test_conversion", &origstr, &origlen,
				&expstr, &explen))
		return NULL;

	obj = Tcl_NewStringObj(origstr, origlen);
	/* if the conversion succeeds we can be sure that at least the data
	 * was converted to something valid in UTF-8 */
	ob = TclObj_ToPy(tclpy, obj);
	if (!ob) {
		printf("Failed with '%s'!\n", origstr);
		return NULL;
	}
	if (!PyUnicode_Check(ob)) {
		PyErr_Format(TestError, "an unicode object was expected, "
				"instead we got a '%s'", ob->ob_type->tp_name);
		Py_DECREF(ob);
		return NULL;
	}

	/* now we verify if it is what we really want */
	uni_size = PyUnicode_GetSize(ob);
	if (uni_size != explen) {
		PyObject_Print(ob, stdout, 0);
		PyErr_Format(TestError, "conversion was not done properly, "
				"expected length was %d, got %d", explen, uni_size);
		Py_DECREF(ob);
		return NULL;
	}
	test = PyUnicode_DecodeUTF8(expstr, explen, "strict");
	PyObject_Cmp(test, ob, &cmpresult);
	if (cmpresult) {
		PyErr_SetString(TestError, "converted obj didn't match test obj");
		Py_DECREF(ob);
		Py_DECREF(test);
		return NULL;
	}

	Py_DECREF(test);
	Py_DECREF(ob);
	Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
	{"test_conversion", test_conversion, METH_VARARGS, NULL},
	{NULL},
};


static struct PyModuleDef _tclnull_to_null = {
	PyModuleDef_HEAD_INIT,		/* m_base */
	"_tclnull_to_null",			/* m_name */
	NULL,						/* m_doc */
	-1,							/* m_size */
	test_methods,				/* m_methods */
	NULL,						/* m_reload */
	NULL,						/* m_traverse */
	NULL,						/* m_clear */
	NULL						/* m_free */
};

PyMODINIT_FUNC
PyInit__tclnull_tonull(void)
{
	PyObject *m, *args, *plumage, *interp;

	if (!(m = PyModule_Create(&_tclnull_to_null)))
		return NULL;

	plumage = PyImport_ImportModule("plumage");
	if (plumage == NULL) {
		Py_DECREF(m);
		return NULL;
	}

	args = PyTuple_New(1);
	Py_INCREF(Py_False);
	PyTuple_SET_ITEM(args, 0, Py_False); /* do not load tk */
	interp = PyObject_GetAttrString(plumage, "Interp");
	if (!interp) {
		Py_DECREF(args);
		Py_DECREF(m);
		return NULL;
	}
	tclpy = (TclInterpObj *)PyObject_CallObject(interp, args);
	Py_DECREF(interp);
	Py_DECREF(args);
	if (!tclpy) {
		Py_DECREF(m);
		return NULL;
	}

	TestError = PyErr_NewException("_tclnull_tonull.error", NULL, NULL);
	Py_INCREF(TestError);
	PyModule_AddObject(m, "error", TestError);

	return m;
}
