// Copyright © 2021 CCP ehf.

#include "stdafx.h"
#include "SpaceMouse.h"


#if _WIN32 && !NO_SPACE_MOUSE

int s_registeredCount = 0;

const char* PROP_DEVICE = "SpaceMouseDevice";
const char* PROP_HANDLER = "SpaceMouseHandler";
const char* PROP_ACTION_HANDLER = "SpaceMouseActionHandler";
const char* PROP_OLD_PROC = "SpaceMouseProc";


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


int RegisterWindow( const char* appName, void* window, PyObject* handler, PyObject* actionHandler )
{
	if( s_registeredCount == 0 )
	{
		auto ret = SiInitialize();
		if( ret != SPW_NO_ERROR )
		{
			return ret;
		}
	}

	auto hwnd = reinterpret_cast<HWND>( window );

	if( auto oldHandler = reinterpret_cast<PyObject*>( GetProp( hwnd, PROP_HANDLER ) ) )
	{
		Py_IncRef( handler );
		Py_DecRef( oldHandler );
		SetProp( hwnd, PROP_HANDLER, reinterpret_cast<HANDLE>( handler ) );

		auto oldActionHandler = reinterpret_cast<PyObject*>( GetProp( hwnd, PROP_ACTION_HANDLER ) );
		Py_XINCREF( actionHandler );
		Py_XDECREF( oldActionHandler );
		SetProp( hwnd, PROP_ACTION_HANDLER, reinterpret_cast<HANDLE>( actionHandler ) );
	}
	else
	{
		SiOpenData data;
		SiOpenWinInit( &data, hwnd );
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
		SetProp( hwnd, PROP_HANDLER, reinterpret_cast<HANDLE>( handler ) );
		Py_XINCREF( actionHandler );
		SetProp( hwnd, PROP_ACTION_HANDLER, reinterpret_cast<HANDLE>( actionHandler ) );
		SetProp( hwnd, PROP_DEVICE, reinterpret_cast<HANDLE>( deviceHandle ) );
		auto oldProc = (WNDPROC)SetWindowLongPtrW( hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( &HandleEvent ) );
		SetProp( hwnd, PROP_OLD_PROC, reinterpret_cast<HANDLE>( oldProc ) );
	}

	return SPW_NO_ERROR;
}

bool UnRegisterWindow( void* window )
{
	auto hwnd = reinterpret_cast<HWND>( window );

	auto oldProc = reinterpret_cast<WNDPROC>( GetProp( hwnd, PROP_OLD_PROC ) );
	if( !oldProc )
	{
		PyErr_SetString( PyExc_ValueError, "window was not registered with space mouse" );
		return false;
	}
	SetWindowLongPtr( hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( oldProc ) );
	RemoveProp( hwnd, PROP_OLD_PROC );

	auto handler = reinterpret_cast<PyObject*>( GetProp( hwnd, PROP_HANDLER ) );
	Py_DecRef( handler );
	RemoveProp( hwnd, PROP_HANDLER );

	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hwnd, PROP_DEVICE ) );
	SiClose( deviceHandle );
	RemoveProp( hwnd, PROP_DEVICE );

	if( --s_registeredCount == 0 )
	{
		SiTerminate();
	}

	return true;
}

void SetSpaceMouseException( const char* context, int errorCode )
{
	if( errorCode != SPW_NO_ERROR )
	{
		if( auto message = SpwErrorString( SpwRetVal( errorCode ) ) )
		{
			PyErr_Format( PyExc_RuntimeError, "%s: code %i - %s", context, errorCode, message );
		}
		else
		{
			PyErr_Format( PyExc_RuntimeError, "%s: code %i", context, errorCode );
		}
	}
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

bool AddActionSet( void* window, PyObject* actions )
{
	auto root = ParseAction( actions );
	if( !root )
	{
		return false;
	}

	root->type = SI_ACTIONSET_NODE;

	auto hWnd = reinterpret_cast<HWND>( window );
	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );

	auto ret = SiAppCmdWriteActions( deviceHandle, root );
	DeleteAction( root );
	if( ret != SPW_NO_ERROR )
	{
		SetSpaceMouseException( "failed to set space mouse actions", ret );
		return false;
	}
	return true;
}

bool ActivateActionSet( void* window, const char* setName )
{
	auto hWnd = reinterpret_cast<HWND>( window );
	auto deviceHandle = reinterpret_cast<SiHdl>( GetProp( hWnd, PROP_DEVICE ) );
	auto ret = SiAppCmdActivateActionSet( deviceHandle, setName );
	if( ret )
	{
		SetSpaceMouseException( "failed to activate action set", ret );
		return false;
	}
	return true;
}

#endif