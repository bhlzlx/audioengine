#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

namespace ow
{
	struct MemBuffer:public owBuffer
	{
		// 指针
		owBYTE*		m_pData;
		owBYTE*		m_pCurr;
		owBYTE*		m_pEnd;
		// 容量
		owSIZE_T	m_nCapacity;
		
		MemBuffer()
		{
			this->m_nCapacity = 0;
			this->m_pCurr = this->m_pData = this->m_pEnd = NULL;
		}
		
		MemBuffer( const owVOID * _pSrc, owINT32 & _nSize)
		{
			m_pData = (owBYTE *)malloc( _nSize + 1);
			memcpy(m_pData, _pSrc, _nSize);
			m_pData[_nSize] = 0x00;
			m_pCurr = m_pData;
			m_pEnd = m_pData + _nSize;
			m_nCapacity = _nSize;
		}
		
		MemBuffer( const owINT32& _nSize)
		{
			m_pData = (owBYTE *)malloc( _nSize + 1);
			m_pCurr = m_pData;
			m_pEnd = m_pData + _nSize;
			m_nCapacity = _nSize;
		}
		
		owSIZE_T Size()
		{
			return m_nCapacity;
		}
		
		owINT32 Seek( owINT8 _flag, owINT32 _offset)
		{
			switch(_flag)
			{
			case SEEK_SET:
				m_pCurr = m_pData + _offset;
				break;
			case SEEK_CUR:
				m_pCurr += _offset;
				break;
			case SEEK_END:
				m_pCurr = m_pEnd + _offset;
				break;
			}
			
			if((m_pEnd - m_pCurr) < 0)
			{
				m_pCurr = m_pEnd;
			}
			else if(m_pCurr < m_pData)
			{
				m_pCurr = m_pData;
			}
			
			return 1;
		}
		
		owINT32 Read( owVOID* _pOut, owINT32 _nSize)
		{
			owINT32 sizeLeft = m_pEnd - m_pCurr;
			if(!sizeLeft)
			{
				return 0;
			}
			if(_nSize > sizeLeft)
			{
				_nSize = sizeLeft;
			}
			memcpy(_pOut,m_pCurr,_nSize);
			m_pCurr += _nSize;
			return _nSize;
		}
		
		owINT32 Resize( owINT32 _nSize)
		{
			assert( _nSize > this->m_nCapacity);
			owINT32 currOffset = m_pCurr - m_pData;
			this->m_pData = (owBYTE *)realloc( m_pData, _nSize + 1);
			assert( m_pData );
			
			m_pData[_nSize] = 0x0;
			m_nCapacity = _nSize;
			m_pCurr = m_pData + currOffset;
			m_pEnd = m_pData + _nSize;
			
			return 0;
		}
		
		owINT32 Write( owVOID* _pIn, owINT32 _nSize)
		{
			owINT32 sizeLeft = m_pEnd - m_pCurr;
			while(sizeLeft < _nSize)
			{
				Resize( m_nCapacity * 2);
				sizeLeft = m_pEnd - m_pCurr;
			}
			memcpy(m_pCurr, _pIn, _nSize);
			m_pCurr += _nSize;
			return _nSize;
		}
		
		owBOOL Eof()
		{
			if(m_pCurr>=m_pEnd)
			{
				return true;
			}
			return false;
		}
		
		owBYTE* GetCurr()
		{
			return m_pCurr;
		}
		
		owBYTE* GetBuffer()
		{
			return m_pData;
		}

		owVOID Release()
		{
			free( m_pData);
			this->m_nCapacity = 0;
			this->m_pCurr = this->m_pData = this->m_pEnd = NULL;
			delete this;
		}
		
		~MemBuffer()
		{
			return ;
		}
	};

	struct MemBufferRef:public MemBuffer
	{
		MemBufferRef( owVOID* _pSrc, owINT32& _nSize)
		{
			m_pData = (owBYTE*)_pSrc;
			m_pCurr = m_pData;
			m_pEnd = m_pData + _nSize;
			m_nCapacity = _nSize;
		}
		
		owINT32 Resize( owINT32 _nSize)
		{
			assert( false );
			return -1;
		}
		
		owVOID Release()
		{
			delete this;
		}
		
		~MemBufferRef()
		{
			
		}
		
	};

	owBuffer * CreateMemBuffer(owINT32 _nSize)
	{
		return new MemBuffer(_nSize);
	}

	owBuffer * BufferFromFile(const owCHAR * _szFile)
	{
		FILE * file = fopen(_szFile,"rb");
		
		if(!file)
		{
			return NULL;
		}
		fseek(file,0,SEEK_END);
		long fileLen = ftell(file);
		fseek(file,0,SEEK_SET);
		MemBuffer * buffer = new MemBuffer(fileLen);
		fread(buffer->m_pData,1,fileLen,file);
		fclose(file);
		return buffer;
	}

	owBuffer * CreateBufferRef( owVOID * _pData, owINT32 _nLength)
	{
		return new MemBufferRef(_pData, _nLength);
	}

}

