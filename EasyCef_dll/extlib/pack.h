#ifndef _PACK_H
#define _PACK_H




#ifdef __cplusplus

class XPackData
{
public:
	virtual operator const char* () = 0;
	virtual operator unsigned char* () const = 0;
	virtual int GetLen() = 0;
	virtual void Free() = 0;
};

BOOL PreLoadPackFile(const WCHAR*);
int UnzipExistPackFile(const WCHAR*, const WCHAR*, XPackData**);
BOOL CleanLoadedPacks(const WCHAR* path); //null for all


extern "C" {
#endif 

extern bool __stdcall zipFile2PackFile( const WCHAR* , const WCHAR* );

extern bool __stdcall exZipFile( const WCHAR* , const WCHAR* , unsigned char ** , unsigned long *);

extern void __stdcall freeExtfileBuf( const char* );

extern void __stdcall freeBuf(const unsigned char*);




#ifdef __cplusplus
}
#endif 


#endif