
#include "WebkitEcho.h"

wrapQweb::EchoMap* WebkitEcho::s_fnMap = NULL;
wrapQweb::FunMap* WebkitEcho::s_uifnMap = NULL;

WebkitEcho::WebkitEcho()
{
	
}


WebkitEcho::~WebkitEcho()
{
}

void WebkitEcho::SetFunMap(wrapQweb::EchoMap* map)
{
	s_fnMap = map;
}

void WebkitEcho::SetUIFunMap(wrapQweb::FunMap* map)
{
	s_uifnMap = map;
}
