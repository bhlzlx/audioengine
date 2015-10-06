#ifndef __OW_BUFFER_H__
#define __OW_BUFFER_H__

#include <owcmn/owcmn.h>

namespace ow
{
	struct owBuffer
	{
		virtual owSIZE_T 	Size() = 0;
		virtual owINT32		Seek( owINT8 _flag, owINT32 _offset) = 0;
		virtual owINT32		Read( owVOID* _pOut, owINT32 _nSize) = 0;
		virtual owINT32		Write( owVOID* _pIn, owINT32 _nSize) = 0;
		/* return 0 if success, or return -1 if failed */
		virtual owINT32 	Resize( owINT32 _nSize) = 0;
		virtual owBOOL 		Eof() = 0;
		
		virtual owBYTE* 	GetCurr() = 0;
		virtual owBYTE* 	GetBuffer() = 0;
		
		virtual owVOID 		Release() = 0;
		
		virtual ~owBuffer(){};
	};

	owBuffer * CreateMemBuffer( owINT32 );
	owBuffer * BufferFromFile( const owCHAR * szFile);
	owBuffer * CreateBufferRef( owVOID * _pData, owINT32 _nLength);
}



#endif // __OW_BUFFER_H__
