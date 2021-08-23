#include "stdafx.h"
#include "SpaceMouse.h"

#if !NO_SPACE_MOUSE

#include <ConnexionClientAPI.h>
#import <Cocoa/Cocoa.h>

extern int16_t SetConnexionHandlers(ConnexionMessageHandlerProc messageHandler, ConnexionAddedHandlerProc addedHandler, ConnexionRemovedHandlerProc removedHandler, bool useSeparateThread) __attribute__((weak_import));


namespace
{
    
uint16_t s_clientID = 0;
int32_t s_registeredCount = 0;
PyObject* s_handler = nullptr;
    
void MessageHandler( unsigned int connection, natural_t messageType, void *messageArgument )
{
	const ConnexionDeviceState* state;
    
    switch( messageType )
    {
    case kConnexionMsgDeviceState:
		state = static_cast<ConnexionDeviceState*>( messageArgument );
        if( state->client == s_clientID )
        {
            switch( state->command )
            {
            case kConnexionCmdHandleAxis:
                SpaceMousePosition pos;
                pos.translation[0] = state->axis[0] / 500.f;
                pos.translation[1] = -state->axis[2] / 500.f;
                pos.translation[2] = -state->axis[1] / 500.f;
                pos.rotation[0] = state->axis[3] / 500.f;
                pos.rotation[1] = -state->axis[5] / 500.f;
                pos.rotation[2] = -state->axis[4] / 500.f;
                PyMoveEventHandler( pos, s_handler );
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
}
}

int RegisterWindow( const char*, void*, PyObject* handler, PyObject* actionHandler )
{
    if( s_clientID )
    {
        ++s_registeredCount;
        Py_XDECREF( s_handler );
        s_handler = handler;
        Py_XINCREF( s_handler );
        return 0;
    }
    if( !SetConnexionHandlers )
    {
        return 101;
    }
    
    auto error = SetConnexionHandlers( MessageHandler, nullptr, nullptr, false );
    if( error )
    {
        return error;
    }
    
    std::vector<char> appPath( 128 );
    uint32_t size = uint32_t( appPath.size() ) - 1;
    if( _NSGetExecutablePath( &appPath[1], &size ) < 0 )
    {
        appPath.resize( size + 1 );
        _NSGetExecutablePath( &appPath[1], &size );
    }
    char* start = strrchr( appPath.data() + 1, '/' );
    if( !start )
    {
        start = appPath.data();
    }
    *start = uint8_t( strlen( start + 1 ) );
	s_clientID = RegisterConnexionClient( 0, reinterpret_cast<uint8_t*>( start ), kConnexionClientModeTakeOver, kConnexionMaskAll );
    
    if( !s_clientID )
    {
        return 103;
    }
    
    SetConnexionClientButtonMask(s_clientID, kConnexionMaskAllButtons);

    Py_XDECREF( s_handler );
    s_handler = handler;
    Py_XINCREF( s_handler );
    ++s_registeredCount;

    return 0;
}

bool UnRegisterWindow( void* )
{
    if( !s_clientID )
    {
        PyErr_SetString( PyExc_ValueError, "application was not registered with space mouse" );
        return false;
    }
    
    if( --s_registeredCount <= 0 )
    {
        UnregisterConnexionClient( s_clientID );
        CleanupConnexionHandlers();
        
        Py_XDECREF( s_handler );
        s_handler = nullptr;
    }
    return true;
}

void SetSpaceMouseException( const char* context, int errorCode )
{
    switch( errorCode )
    {
    case 0:
        return;
    case 101:
        PyErr_Format( PyExc_RuntimeError, "%s: could not find space mouse driver", context );
        return;
    case 102:
        PyErr_Format( PyExc_ValueError, "%s: application name is too long", context );
        return;
    case 103:
        PyErr_Format( PyExc_RuntimeError, "%s: could not register application with space mouse driver", context );
        return;
    default:
        PyErr_Format( PyExc_RuntimeError, "%s: driver error code %i", context, errorCode );
    }
}

bool AddActionSet( void* window, PyObject* actions )
{
    // OSX drivers don't support programmable actions
    return true;
}

bool ActivateActionSet( void* window, const char* setName )
{
    return true;
}

#endif