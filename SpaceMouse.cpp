#include "stdafx.h"
#include "SpaceMouse.h"


void PyMoveEventHandler( const SpaceMousePosition& pos, void* context )
{
	auto gstate = PyGILState_Ensure();

	auto tuple = PyTuple_New( 6 );
	PyTuple_SET_ITEM( tuple, 0, PyFloat_FromDouble( pos.translation[0] ) );
	PyTuple_SET_ITEM( tuple, 1, PyFloat_FromDouble( pos.translation[1] ) );
	PyTuple_SET_ITEM( tuple, 2, PyFloat_FromDouble( pos.translation[2] ) );
	PyTuple_SET_ITEM( tuple, 3, PyFloat_FromDouble( pos.rotation[0] ) );
	PyTuple_SET_ITEM( tuple, 4, PyFloat_FromDouble( pos.rotation[1] ) );
	PyTuple_SET_ITEM( tuple, 5, PyFloat_FromDouble( pos.rotation[2] ) );

	auto result = PyObject_Call( (PyObject*)context, tuple, nullptr );
	Py_XDECREF( result );
	Py_DecRef( tuple );
	PyGILState_Release( gstate );
}

void PyActionEventHandler( const char* id, void* context )
{
	auto gstate = PyGILState_Ensure();

	auto tuple = PyTuple_New( 1 );
	PyTuple_SET_ITEM( tuple, 0, PyString_FromString( id ) );

	auto result = PyObject_Call( (PyObject*)context, tuple, nullptr );
	if( result )
	{
		Py_DecRef( result );
	}
	Py_DecRef( tuple );
	PyGILState_Release( gstate );
}

#if NO_SPACE_MOUSE

int RegisterWindow( const char* appName, void* window, PyObject* handler, PyObject* actionHandler )
{
	return 0;
}

bool UnRegisterWindow( void* window )
{
	PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
	return false;
}

void SetSpaceMouseException( const char* context, int errorCode )
{
}

bool AddActionSet( void* window, PyObject* actions )
{
	PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
	return false;
}

bool ActivateActionSet( void* window, const char* setName )
{
	PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
	return false;
}

#endif

PyObject* PyRegisterWindow( PyObject* self, PyObject* args )
{
	const char* name;
	unsigned long long wnd;
	PyObject* handler = nullptr;
	PyObject* actionHandler = nullptr;
	if( !PyArg_ParseTuple( args, "sKO|O", &name, &wnd, &handler, &actionHandler ) )
	{
		return nullptr;
	}
	if( !PyCallable_Check( handler ) )
	{
		PyErr_SetString( PyExc_TypeError, "expected a callable handler argument" );
		return nullptr;
	}
	if( actionHandler && !PyCallable_Check( actionHandler ) )
	{
		PyErr_SetString( PyExc_TypeError, "expected a callable action handler argument" );
		return nullptr;
	}

	auto ret = RegisterWindow( name, reinterpret_cast<void*>( wnd ), handler, actionHandler );
	if( !ret )
	{
		PyMoveEventHandler( SpaceMousePosition(), handler );
		Py_RETURN_NONE;
	}
	else
	{
		SetSpaceMouseException( "failed to initialize space mouse", ret );
		return nullptr;
	}
}

PyObject* PyUnRegisterWindow( PyObject* self, PyObject* args )
{
	unsigned long long wnd;
	if( !PyArg_ParseTuple( args, "K", &wnd ) )
	{
		return nullptr;
	}
	if( !UnRegisterWindow( reinterpret_cast<void*>( wnd ) ) )
	{
		PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
		return nullptr;
	}
	Py_RETURN_NONE;
}

PyObject* PyAddActionSet( PyObject* self, PyObject* args )
{
	PyObject* pyActions;
	unsigned long long wnd;
	if( !PyArg_ParseTuple( args, "KO", &wnd, &pyActions ) )
	{
		return nullptr;
	}

	if( AddActionSet( reinterpret_cast<void*>( wnd ), pyActions ) )
	{
		Py_RETURN_NONE;
	}
	else
	{
		return nullptr;
	}
}

PyObject* PyActivateActionSet( PyObject* self, PyObject* args )
{
	unsigned long long wnd;
	const char* name;
	if( !PyArg_ParseTuple( args, "Ks", &wnd, &name ) )
	{
		return nullptr;
	}

	if( ActivateActionSet( reinterpret_cast<void*>( wnd ), name ) )
	{
		Py_RETURN_NONE;
	}
	else
	{
		return nullptr;
	}
}


PyMethodDef SpaceMouseMethods[] = {
	{
		"RegisterWindow",
		PyRegisterWindow,
		METH_VARARGS,
		"Registers a window to receive space mouse events.\n"
		":param name: application name\n"
		":type name: string\n"
		":param wnd: window handle\n"
		":type wnd: long\n"
		":param handler: space mouse message handler callable\n"
		":type handler: ((float, float, float), (float, float, float)) -> None\n"
		":param actionHandler: space mouse action handler callable\n"
		":type actionHandler: (str) -> None\n"
	},
	{
		"UnRegisterWindow",
		PyUnRegisterWindow,
		METH_VARARGS,
		"Registers a window to receive space mouse events.\n"
		":param wnd: window handle\n"
		":type wnd: long\n"
	},
	{
		"AddActionSet",
		PyAddActionSet,
		METH_VARARGS,
		"Registers an action set for space mouse buttons. Action set is described as (id, label, help, actions) tuple.\n"
		"Here id is a unique identifier id for the action set, label - localized action set label, help - localized\n"
		"help string for the action set, actions - a sequence of subactions with (id, label, help) format or action\n"
		"groups with (id, label, help, actions) format.\n"
		":param wnd: window handle\n"
		":type wnd: long\n"
		":param actionSet: application action set description\n"
		":type actionSet: (str, str, str, list)"
	},
	{
		"ActivateActionSet",
		PyActivateActionSet,
		METH_VARARGS,
		"Activates previously added action set.\n"
		":param wnd: window handle\n"
		":type wnd: long\n"
		":param name: action set name (id)\n"
		":type name: str\n"
	},
	{ nullptr, nullptr, 0, nullptr }
};

#define CONCAT( a, b ) _CONCAT( a, b )
#define _CONCAT( a, b ) a##b
#define STRINGIZE( x ) _STRINGIZE( x )
#define _STRINGIZE( s ) #s

#ifndef _WIN32
__attribute__((visibility ("default")))
#endif
PyMODINIT_FUNC CONCAT( init_spacemouse, CCP_BUILD_FLAVOR )()
{
	Py_InitModule( STRINGIZE( CONCAT( _spacemouse, CCP_BUILD_FLAVOR ) ), SpaceMouseMethods );
}
