#ifndef _webkitecho_h_
#define _webkitecho_h_
#pragma once


class WebkitEcho
{
public:
	WebkitEcho();
	virtual ~WebkitEcho();

	static void SetFunMap(wrapQweb::EchoMap*);
	static const wrapQweb::EchoMap* getFunMap(){
		return s_fnMap;
	}

	static void SetUIFunMap(wrapQweb::FunMap*);
	static const wrapQweb::FunMap* getUIFunMap() {
		return s_uifnMap;
	}

private:
	static wrapQweb::EchoMap* s_fnMap;

	static wrapQweb::FunMap* s_uifnMap;
};

#endif
