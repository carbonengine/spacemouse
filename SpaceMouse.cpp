#include "stdafx.h"


struct SpaceMousePosition
{
	float translation[3];
	float rotation[3];
};

int s_registeredCount = 0;

const char* PROP_DEVICE = "SpaceMouseDevice";
const char* PROP_HANDLER = "SpaceMouseHandler";
const char* PROP_ACTION_HANDLER = "SpaceMouseActionHandler";
const char* PROP_OLD_PROC = "SpaceMouseProc";

SpwRetVal RegisterWindow( const char* appName, HWND window, PyObject* handler, PyObject* actionHandler );
bool UnRegisterWindow( HWND hWnd );


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

LRESULT WINAPI HandleEvent( HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam )
{
	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );
	if( deviceHandle )
	{
		SiGetEventData EData;    /* SpaceWare Event Data */
		SiGetEventWinInit( &EData, msg, wParam, lParam );

		SiSpwEvent event;

		/* check whether msg was a 3D mouse event and process it */
		if( SiGetEvent( deviceHandle, SI_AVERAGE_EVENTS, &EData, &event ) == SI_IS_EVENT )
		{
			switch( event.type )
			{
			case SI_MOTION_EVENT:
			{
				auto handler = reinterpret_cast<PyObject*>( GetProp( hWnd, PROP_HANDLER ) );

				SpaceMousePosition pos;
				pos.translation[0] = event.u.spwData.mData[SI_TX] / 500.f;
				pos.translation[1] = event.u.spwData.mData[SI_TY] / 500.f;
				pos.translation[2] = event.u.spwData.mData[SI_TZ] / 500.f;
				pos.rotation[0] = event.u.spwData.mData[SI_RX] / 500.f;
				pos.rotation[1] = event.u.spwData.mData[SI_RY] / 500.f;
				pos.rotation[2] = event.u.spwData.mData[SI_RZ] / 500.f;
				PyMoveEventHandler( pos, handler );
				break;
			}
			case SI_ZERO_EVENT:
			{
				auto handler = reinterpret_cast<PyObject*>( GetProp( hWnd, PROP_HANDLER ) );

				SpaceMousePosition pos;
				pos.translation[0] = 0;
				pos.translation[1] = 0;
				pos.translation[2] = 0;
				pos.rotation[0] = 0;
				pos.rotation[1] = 0;
				pos.rotation[2] = 0;
				PyMoveEventHandler( pos, handler );
				break;
			}
			case SI_APP_EVENT:
			{
				if( event.u.appCommandData.pressed )
				{
					if( auto handler = reinterpret_cast<PyObject*>( GetProp( hWnd, PROP_ACTION_HANDLER ) ) )
					{
						PyActionEventHandler( event.u.appCommandData.id.appCmdID, handler );
					}
				}
				break;
			}
			default:
				break;
			}

			return 0;
		}
	}

	auto proc = reinterpret_cast<WNDPROC>( GetProp( hWnd, PROP_OLD_PROC ) );
	auto result = CallWindowProcW( proc, hWnd, msg, wParam, lParam );
	if( msg == WM_DESTROY )
	{
		auto gstate = PyGILState_Ensure();
		UnRegisterWindow( hWnd );
		PyGILState_Release( gstate );
	}
	return result;
}


SpwRetVal RegisterWindow( const char* appName, HWND window, PyObject* handler, PyObject* actionHandler )
{
	if( s_registeredCount == 0 )
	{
		auto ret = SiInitialize();
		if( ret != SPW_NO_ERROR )
		{
			return ret;
		}
	}

	if( auto oldHandler = reinterpret_cast<PyObject*>( GetProp( window, PROP_HANDLER ) ) )
	{
		Py_IncRef( handler );
		Py_DecRef( oldHandler );
		SetProp( window, PROP_HANDLER, reinterpret_cast<HANDLE>( handler ) );

		auto oldActionHandler = reinterpret_cast<PyObject*>( GetProp( window, PROP_ACTION_HANDLER ) );
		Py_XINCREF( actionHandler );
		Py_XDECREF( oldActionHandler );
		SetProp( window, PROP_ACTION_HANDLER, reinterpret_cast<HANDLE>( actionHandler ) );
	}
	else
	{
		SiOpenData data;
		SiOpenWinInit( &data, window );
		auto deviceHandle = SiOpen( appName, SI_ANY_DEVICE, SI_NO_MASK, SI_EVENT, &data );
		if( !deviceHandle )
		{
			if( s_registeredCount == 0 )
			{
				SiTerminate();
			}
			return SPW_ERROR;
		}

		s_registeredCount++;
		Py_IncRef( handler );
		SetProp( window, PROP_HANDLER, reinterpret_cast<HANDLE>( handler ) );
		Py_XINCREF( actionHandler );
		SetProp( window, PROP_ACTION_HANDLER, reinterpret_cast<HANDLE>( actionHandler ) );
		SetProp( window, PROP_DEVICE, reinterpret_cast<HANDLE>( deviceHandle ) );
		auto oldProc = (WNDPROC)SetWindowLongPtrW( window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( &HandleEvent ) );
		SetProp( window, PROP_OLD_PROC, reinterpret_cast<HANDLE>( oldProc ) );
	}

	return SPW_NO_ERROR;
}

bool UnRegisterWindow( HWND hWnd )
{
	auto oldProc = reinterpret_cast<WNDPROC>( GetProp( hWnd, PROP_OLD_PROC ) );
	if( !oldProc )
	{
		PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
		return false;
	}
	SetWindowLongPtr( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( oldProc ) );
	RemoveProp( hWnd, PROP_OLD_PROC );

	auto handler = reinterpret_cast<PyObject*>( GetProp( hWnd, PROP_HANDLER ) );
	Py_DecRef( handler );
	RemoveProp( hWnd, PROP_HANDLER );

	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );
	SiClose( deviceHandle );
	RemoveProp( hWnd, PROP_DEVICE );

	if( --s_registeredCount == 0 )
	{
		SiTerminate();
	}

	return true;
}

void DeleteAction( SiActionNodeEx_t* action )
{
	if( action->children )
	{
		DeleteAction( action->children );
	}
	if( action->next )
	{
		DeleteAction( action->next );
	}
	delete action;
}

bool ParseActionSequence( PyObject* pyActions, SiActionNodeEx_t*& firstAction );


SiActionNodeEx_t* ParseAction( PyObject* pyItem )
{
	if( !pyItem || !PyTuple_Check( pyItem ) )
	{
		PyErr_SetString( PyExc_ValueError, "each action must be a 3- or 4- tuple" );
		return nullptr;
	}

	const char* id;
	const char* label;
	const char* help;
	PyObject* subactions = nullptr;

	if( !PyArg_ParseTuple( pyItem, "sss|O;each action must be a tuple (id, label, help, [subactions])", &id, &label, &help, &subactions ) )
	{
		return nullptr;
	}

	auto action = new SiActionNodeEx_t();
	action->size = sizeof( SiActionNodeEx_t );
	action->type = subactions ? SI_CATEGORY_NODE : SI_ACTION_NODE;
	action->id = id;
	action->label = label;
	action->description = subactions ? nullptr : help;
	action->next = nullptr;
	action->children = nullptr;
	if( subactions && !ParseActionSequence( subactions, action->children ) )
	{
		DeleteAction( action );
		return nullptr;
	}

	return action;
}

bool ParseActionSequence( PyObject* pyActions, SiActionNodeEx_t*& firstAction )
{
	if( !PySequence_Check( pyActions ) )
	{
		PyErr_SetString( PyExc_ValueError, "expected a sequence of actions" );
		return false;
	}

	auto count = PySequence_Size( pyActions );
	firstAction = nullptr;
	SiActionNodeEx_t* prevAction = nullptr;
	for( Py_ssize_t i = 0; i < count; ++i )
	{
		auto pyItem = PySequence_GetItem( pyActions, i );
		auto action = ParseAction( pyItem );
		Py_XDECREF( pyItem );
		if( !action )
		{
			return false;
		}
		if( prevAction )
		{
			prevAction->next = action;
		}
		prevAction = action;

		if( !firstAction )
		{
			firstAction = action;
		}
	}
	return true;
}

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

	auto ret = RegisterWindow( name, (HWND)wnd, handler, actionHandler );
	if( ret == SPW_NO_ERROR )
	{
		PyMoveEventHandler( SpaceMousePosition(), handler );
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_Format( PyExc_RuntimeError, "failed to initialize space mouse, error code: %i", int( ret ) );
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

	auto hWnd = reinterpret_cast<HWND>( wnd );
	if( !UnRegisterWindow( hWnd ) )
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

	SiActionNodeEx_t* root = ParseAction( pyActions );
	if( !root )
	{
		return nullptr;
	}

	root->type = SI_ACTIONSET_NODE;

	auto hWnd = reinterpret_cast<HWND>( wnd );
	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );

	auto ret = SiAppCmdWriteActions( deviceHandle, root );
	DeleteAction( root );
	if( ret != SPW_NO_ERROR )
	{
		PyErr_Format( PyExc_RuntimeError, "failed to set space mouse actions: %s", SpwErrorString( ret ) );
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* PyActivateActionSet( PyObject* self, PyObject* args )
{
	unsigned long long wnd;
	const char* name;
	if( !PyArg_ParseTuple( args, "Ks", &wnd, &name ) )
	{
		return nullptr;
	}
	auto hWnd = reinterpret_cast<HWND>( wnd );
	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );
	auto ret = SiAppCmdActivateActionSet( deviceHandle, name );
	if( ret != SPW_NO_ERROR )
	{
		PyErr_Format( PyExc_RuntimeError, "failed to activate action set: %s", SpwErrorString( ret ) );
		return nullptr;
	}

	Py_RETURN_NONE;
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


PyMODINIT_FUNC initspacemouse( void )
{
	Py_InitModule( "spacemouse", SpaceMouseMethods );
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}
