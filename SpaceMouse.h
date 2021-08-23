#pragma once

// SpaceMouse currently isn't available for ARM64
#if __aarch64__
#ifdef NO_SPACE_MOUSE
#undef NO_SPACE_MOUSE
#endif
#define NO_SPACE_MOUSE 1
#endif

struct SpaceMousePosition
{
	float translation[3];
	float rotation[3];
};

int RegisterWindow( const char* appName, void* window, PyObject* handler, PyObject* actionHandler );
bool UnRegisterWindow( void* window );
void SetSpaceMouseException( const char* context, int errorCode );
bool AddActionSet( void* window, PyObject* actions );
bool ActivateActionSet( void* window, const char* setName );

void PyMoveEventHandler( const SpaceMousePosition& pos, void* context );
void PyActionEventHandler( const char* id, void* context );
