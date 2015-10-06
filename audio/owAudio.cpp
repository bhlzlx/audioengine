#include "owAudio.h"
#include <memory.h>
#include <owcmn/owcmn.h>
#include <buffer/buffer.h>
#include <memory.h>

namespace ow
{
	
	#define FRAME_BUFFER_TIME_MS 500.0f
	
	struct RIFF_HEADER
	{
		owCHAR szRiffID[4];  // 'R','I','F','F'
		owINT32 dwRiffSize;
		owCHAR szRiffFormat[4]; // 'W','A','V','E'
	};

	struct WAVE_FORMAT
	{
		owINT16 wFormatTag;
		owINT16 wChannels;
		owINT32 dwSamplesPerSec;
		owINT32 dwAvgBytesPerSec;
		owINT16 wBlockAlign;
		owINT16 wBitsPerSample;
	};

	struct FMT_BLOCK
	{
		owCHAR  szFmtID[4]; // 'f','m','t',' '
		owINT32  dwFmtSize;
		WAVE_FORMAT wavFormat;
	};

	struct CHUNK_INFO
	{
		owCHAR  szChunkID[4];
		owINT32  dwChunkSize;
	};

	struct DATA_CHUNK
	{
		ALsizei					nDataSize;
		ALchar*					pData;
	};

	struct WAVE_INFO
	{
		DATA_CHUNK	m_DataChunk;
		owINT32		m_format;
		owINT32		m_samplePerSec;
	};
	
	int WAVEFormat2AL(FMT_BLOCK * pFMT)
	{
		ALenum result = 0;
		if (pFMT->wavFormat.wChannels == 1)
		{
			switch(pFMT->wavFormat.wBitsPerSample)
			{
			case 4:
				result = alGetEnumValue("AL_FORMAT_MONO_IMA4");
				break;
			case 8:
				result = alGetEnumValue("AL_FORMAT_MONO8");
				break;
			case 16:
				result = alGetEnumValue("AL_FORMAT_MONO16");
				break;
			}
		}
		else if (pFMT->wavFormat.wChannels == 2)
		{
			switch(pFMT->wavFormat.wBitsPerSample)
			{
			case 4:
				result = alGetEnumValue("AL_FORMAT_STEREO4");
				break;
			case 8:
				result = alGetEnumValue("AL_FORMAT_STEREO8");
				break;
			case 16:
				result = alGetEnumValue("AL_FORMAT_STEREO16");
				break;
			}
		}
		else if(pFMT->wavFormat.wChannels == 4 && pFMT->wavFormat.wBitsPerSample == 16)
		{
			result = alGetEnumValue("AL_FORMAT_QUAD16");
		}
		return result;
	}
	
	owBOOL LoadWAVEAudio( owBuffer * _pBuffer , WAVE_INFO * info)
	{
		// read header information
		RIFF_HEADER riffHeader;
		_pBuffer->Read( &riffHeader, sizeof(riffHeader));
		if (memcmp(riffHeader.szRiffID,"RIFF",4)!= 0 || memcmp(riffHeader.szRiffFormat,"WAVE",4)!= 0)
		{
			return owFALSE;
		}
		FMT_BLOCK fmtblock;
		_pBuffer->Read(&fmtblock, sizeof(FMT_BLOCK) );
		if (memcmp(fmtblock.szFmtID,"fmt ",4) != 0)
		{
			return owFALSE;
		}
		// fmtBlock 大小只有16或者18这两种可能
		if (fmtblock.dwFmtSize == 16 || fmtblock.dwFmtSize== 18)
		{
			if (fmtblock.dwFmtSize == 18)
			{
				unsigned int addition;
				_pBuffer->Read(&addition,sizeof(unsigned int));
			}
		}
		else
		{
			return owFALSE;
		}
		// 
		info->m_format = WAVEFormat2AL(&fmtblock);
		info->m_samplePerSec = fmtblock.wavFormat.dwSamplesPerSec;
		// read data blocks
		CHUNK_INFO chunkInfo;
		while (_pBuffer->Read(&chunkInfo,sizeof(chunkInfo)) == sizeof(chunkInfo))
		{
			// get audio buffer data only
			if (memcmp(chunkInfo.szChunkID,"data",4) == 0)
			{
				DATA_CHUNK  &dataChunk = info->m_DataChunk;
				dataChunk.nDataSize = chunkInfo.dwChunkSize;
				dataChunk.pData = new owCHAR[dataChunk.nDataSize];
				if(_pBuffer->Read(dataChunk.pData,dataChunk.nDataSize) != dataChunk.nDataSize)
				{
					return owFALSE;
				}
				break;
			}
			else
			{
				// 跳过这片数据区域
				_pBuffer->Seek(SEEK_CUR ,chunkInfo.dwChunkSize);
			}
		}
		return owTRUE;
	}
	
	size_t owAEVorbisRead( void * _pData, size_t _nElement, size_t _nCount, void * _pFile)
	{
		ow::owBuffer * pBuffer = (ow::owBuffer *)_pFile;
		return pBuffer->Read( _pData, _nCount * _nElement);
	}

	int owAEVorbisSeek( void * _pFile, ogg_int64_t _nOffset, int _flag )
	{
		ow::owBuffer * pBuffer = (ow::owBuffer *)_pFile;
		return pBuffer->Seek( _flag, _nOffset);
	}
	
	long owAEVorbisTell( void * _pFile)
	{
		ow::owBuffer * pBuffer = (ow::owBuffer *)_pFile;
		return pBuffer->GetCurr() - pBuffer->GetBuffer();
	}
	
	static ov_callbacks OW_CALLBACKS_VORBIS = {
		(size_t (*)(void *, size_t, size_t, void *))  owAEVorbisRead,
		(int (*)(void *, ogg_int64_t, int))           owAEVorbisSeek,
		(int (*)(void *))                             NULL,
		(long (*)(void *))                            owAEVorbisTell
	};
	
	AL_FLOAT_GETTER owAESourceGetter_float = alGetSourcef;
	AL_FLOAT_SETTER owAESourceSetter_float = alSourcef;
	AL_FLOAT_VEC_SETTER owAESourceSetter_vec_float = alSourcefv;
	AL_FLOAT_GETTER owAESourceGetter_vec_float = alGetSourcefv;

	AL_INT32_GETTER owAESourceGetter_int = alGetSourcei;
	AL_INT32_SETTER owAESourceSetter_int = alSourcei;
	AL_INT32_VEC_SETTER owAESourceSetter_vec_int = alSourceiv;
	AL_INT32_GETTER owAESourceGetter_vec_int = alGetSourceiv;

	owVOID owAEBuffer::BufferData(owINT32 _format, const owVOID* _pData, owINT32 _size, owINT32 _freq)
	{
		__CLEAR_AL_ERROR__
		alBufferData( m_iBuffer, _format, _pData, _size, _freq);
		__CHECK_AL_ERROR__
	}

	owINT32 owAEBuffer::GetChannels()
	{
		return this->m_nChannels;
	}

	owINT32 owAEBuffer::GetDepthBits()
	{
		return this->m_nFrequency;
	}

	owINT32 owAEBuffer::GetFrequency()
	{
		return this->m_nFrequency;
	}

	owINT32 owAEBuffer::GetSize()
	{
		return this->m_nSize;
	}

	owBOOL owAEBuffer::Init()
	{
		__CLEAR_AL_ERROR__
		alGenBuffers(1, &this->m_iBuffer);
		__CHECK_AL_ERROR__
		return owTRUE;
	}

	owBOOL owAEBuffer::Valid()
	{
		return alIsBuffer( m_iBuffer);
	}
	
	owAEBuffer::~owAEBuffer()
	{
	}
	
	owVOID owAEBuffer::Release()
	{
		if( Valid() == owTRUE)
		{
			alDeleteBuffers(1, &m_iBuffer);
		}
		memset(this, 0, sizeof(owAEBuffer));
	}
	
	/***************************************************************
	 * 
	 * 
	 * 
	 * ************************************************************/
	 
	owVOID VorbisStruct::Clear()
	{
		// 清除缓冲区
		delete []this->m_pFrameBuffer;
		// 释放OGG文件缓冲
		this->m_pBuffer->Release();
		// 释放OGG文件对象
		ov_clear( &this->m_vorbisFile);
	}
	 
	owINT32 VorbisStruct::Size()
	{
		return ov_raw_total( &this->m_vorbisFile, -1 );
	}
	
	owINT32 VorbisStruct::Tell()
	{
		return ov_raw_tell( &this->m_vorbisFile);
	}
	 
	 owINT32 VorbisStruct::DecodeNextFrame()
	 {
		owINT32 size = 0;
		owINT32 result;
		owINT32 section;
		while(size< this->m_nFrameBufferSize)
		{
			result=ov_read(
			&this->m_vorbisFile,
			this->m_pFrameBuffer+size,
			this->m_nFrameBufferSize-size,
			0,
			2,
			1,
			&section
			);
			// 文件尾
			if(result == 0)
			{
				break;
			}
			// 正常读取
			else if( result > 0)
			{
				size=result+size;
			}
			// 发生错误
			else
			{
				assert( result != OV_HOLE && result != OV_EBADLINK && result != OV_EINVAL);
				break;
			}
		}
		return size;
	 }
	
	owBOOL VorbisStruct::Eof()
	{
		return ( this->Size() == this->Tell() );
	}
	
	owBOOL owAESource::Init()
	{
		__CLEAR_AL_ERROR__
		alGenSources( 1, &m_iSource);
		__CHECK_AL_ERROR__
	}
	
	owBOOL owAESource::Valid()
	{
		return alIsSource( m_iSource);
	}
	
	owVOID owAESource::Release()
	{
		if( Valid() )
		{
			this->m_pBuffer->Release();
			alDeleteSources( 1, &m_iSource );
		}
		memset( this, 0, sizeof( owAESource));
	}

	owVOID owAESource::Pause()
	{
		__CLEAR_AL_ERROR__
		alSourcePause( m_iSource);
		__CHECK_AL_ERROR__
	}

	owVOID owAESource::Play()
	{
		__CLEAR_AL_ERROR__
		alSourcePlay( m_iSource);
		__CHECK_AL_ERROR__
	}

	owVOID owAESource::SetBuffer(owAEBuffer* _pBuffer)
	{
		__CLEAR_AL_ERROR__
		alSourcei( m_iSource, AL_BUFFER, _pBuffer->m_iBuffer);
		this->m_pBuffer = _pBuffer;
		__CHECK_AL_ERROR__
	}

	owVOID owAESource::Stop()
	{
		__CLEAR_AL_ERROR__
		alSourceStop( m_iSource);
		__CHECK_AL_ERROR__
	}
	
	owVOID owAESource::SetFloat( float* _pValue, owINT32 _attr)
	{
		alSourcefv( m_iSource, _attr, _pValue);
	}
	
	owVOID owAESource::SetInt( int* _pValue, owINT32 _attr)
	{
		alSourceiv( m_iSource, _attr, _pValue);
	}
	
	owVOID owAESource::GetFloat( float * _pValue, owINT32 _attr)
	{
		alGetSourcefv( m_iSource, _attr, _pValue);
	}
	
	owVOID owAESource::GetInt( int * _pValue, owINT32 _attr)
	{
		alGetSourceiv( m_iSource, _attr, _pValue);
	}
	
	AL_SOURCE_GET_SET_IMP( owAESource, float, PITCH)
	AL_SOURCE_GET_SET_IMP( owAESource, float, GAIN)
	AL_SOURCE_GET_SET_IMP( owAESource, float, MAX_DISTANCE)
	AL_SOURCE_GET_SET_IMP( owAESource, float, ROLLOFF_FACTOR)
	AL_SOURCE_GET_SET_IMP( owAESource, float, REFERENCE_DISTANCE)
	AL_SOURCE_GET_SET_IMP( owAESource, float, MIN_GAIN)
	AL_SOURCE_GET_SET_IMP( owAESource, float, MAX_GAIN)
	AL_SOURCE_GET_SET_IMP( owAESource, float, CONE_OUTER_GAIN)
	AL_SOURCE_GET_SET_IMP( owAESource, float, CONE_INNER_ANGLE)
	AL_SOURCE_GET_SET_IMP( owAESource, float, CONE_OUTER_ANGLE)
	AL_SOURCE_GET_SET_IMP( owAESource, float, SEC_OFFSET)
	AL_SOURCE_GET_SET_IMP( owAESource, float, BYTE_OFFSET)
	
	AL_SOURCE_GET_SET_IMP( owAESource, int, SOURCE_RELATIVE)
	AL_SOURCE_GET_SET_IMP( owAESource, int, SOURCE_TYPE);
	AL_SOURCE_GET_SET_IMP( owAESource, int, LOOPING);
	AL_SOURCE_GET_SET_IMP( owAESource, int, BUFFER);
	AL_SOURCE_GET_SET_IMP( owAESource, int, SOURCE_STATE);
	AL_SOURCE_GET_SET_IMP( owAESource, int, BUFFERS_QUEUED);
	AL_SOURCE_GET_SET_IMP( owAESource, int, BUFFERS_PROCESSED);
	
	AL_SOURCE_GET_SET_VEC_IMP( owAESource, float, POSITION);
	AL_SOURCE_GET_SET_VEC_IMP( owAESource, float, VELOCITY);
	AL_SOURCE_GET_SET_VEC_IMP( owAESource, float, DIRECTION);

	/***********************************************************
	 * 
	 * 	AEVorbisSource
	 * 
	 * ********************************************************/

	owVOID ow::owAEVorbisSource::FillFirstCycle()
	{
		__CLEAR_AL_ERROR__
		owINT32 size;
		size = this->m_vorbis.DecodeNextFrame();
		alBufferData(
				this->m_vecBuffers[0],
				this->m_vorbis.m_iFormat,
				this->m_vorbis.m_pFrameBuffer,
				size,
				this->m_vorbis.m_pVorbisInfo->rate
				);
		size = this->m_vorbis.DecodeNextFrame();
		alBufferData(
				this->m_vecBuffers[1],
				this->m_vorbis.m_iFormat,
				this->m_vorbis.m_pFrameBuffer,
				size,
				this->m_vorbis.m_pVorbisInfo->rate
				);
		size = this->m_vorbis.DecodeNextFrame();
		alBufferData(
				this->m_vecBuffers[2],
				this->m_vorbis.m_iFormat,
				this->m_vorbis.m_pFrameBuffer,
				size,
				this->m_vorbis.m_pVorbisInfo->rate
				);
		alSourceQueueBuffers( m_iSource,3,&m_vecBuffers[0]);
		__CHECK_AL_ERROR__
	}
	
	owBOOL owAEVorbisSource::UpdateBuffer()
	{
		owINT32  num = 0;
		owINT32  size;
		ALuint bufferarray[3];
		// 取出播放完的buffer
		alGetSourcei(m_iSource,AL_BUFFERS_PROCESSED,&num);
		alSourceUnqueueBuffers(m_iSource,num,&bufferarray[0]);
		// 更新播放完的buffer
		for(int i=0;i<num;i++)
		{
			size=m_vorbis.DecodeNextFrame();
			alBufferData(
				bufferarray[i],
				this->m_vorbis.m_iFormat,
				this->m_vorbis.m_pFrameBuffer,
				size,
				this->m_vorbis.m_pVorbisInfo->rate
				);
		}
		// 将更新的buffer重新装载进source
		alSourceQueueBuffers(m_iSource,num,&bufferarray[0]);
	}

	owVOID ow::owAEVorbisSource::Init(owBuffer* _pBuffer)
	{
		// 从owBuffer读取OGG信息
		owUINT32 error;
		alGenSources( 1, &m_iSource);
		alGenBuffers( 3, &m_vecBuffers[0]);
		error = ov_open_callbacks( _pBuffer, &this->m_vorbis.m_vorbisFile,NULL,0,OW_CALLBACKS_VORBIS);
		if( error != 0)
		{
			assert(false);
			return;
		}
		this->m_vorbis.m_pBuffer = _pBuffer;
		// 读取信息与注释
		this->m_vorbis.m_pVorbisInfo = ov_info( &this->m_vorbis.m_vorbisFile, -1);
		this->m_vorbis.m_pVorbisComment = ov_comment( &this->m_vorbis.m_vorbisFile, -1);
		// 分析音频 format
		/******************************************************
			这里解释一下代码
			PCM数据大小计算 每秒大小 = 采样率 * 位宽 / 8 * 声道数
			位宽是16,声道数是1即
			rate * 16 / 8 * 1
			即 rate * 2 这是一秒的大小
			250ms对应的大小应该是rate / 2
			所以这里就是rate >> 1,位操作右移一位即可实现除2
			但是最后还有个block对齐,这里因为是单声道,16位,两个字节的对齐
		******************************************************/
		// ogg格式位宽全部是16位，参见以下CASE
		// 求一下单声道大小,然后根据声道数再乘
		this->m_vorbis.m_nFrameBufferSize = this->m_vorbis.m_pVorbisInfo->rate * 2 * FRAME_BUFFER_TIME_MS / 1000.0f;
		switch(this->m_vorbis.m_pVorbisInfo->channels)
		{
			// 单声道
			case 1:
				this->m_vorbis.m_iFormat=AL_FORMAT_MONO16;
				//this->m_vorbis.m_nFrameBufferSize;
				break;
			// 双声道
			case 2:
				this->m_vorbis.m_iFormat=AL_FORMAT_STEREO16;
				this->m_vorbis.m_nFrameBufferSize*=2;
				break;
			// 四声道
			case 4:
				this->m_vorbis.m_iFormat=alGetEnumValue("AL_FORMAT_QUAD16");
				this->m_vorbis.m_nFrameBufferSize*=4;
				break;
			// 6声道
			case 6:
				this->m_vorbis.m_iFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				this->m_vorbis.m_nFrameBufferSize*=6;
				break;
			default:
				ov_clear(&m_vorbis.m_vorbisFile);
				return;
		}
		this->m_vorbis.m_pFrameBuffer = new owCHAR[ this->m_vorbis.m_nFrameBufferSize];
		this->FillFirstCycle();
	}

	owVOID ow::owAEVorbisSource::Pause()
	{
		alSourcePause( m_iSource);
	}

	owVOID ow::owAEVorbisSource::Play()
	{
		alSourcePlay( m_iSource);
	}

	owVOID ow::owAEVorbisSource::Release()
	{
		alDeleteBuffers( 3, &this->m_vecBuffers[0]);
		alDeleteSources( 1, &this->m_iSource);
		this->m_vorbis.Clear();
	}

	owVOID ow::owAEVorbisSource::Stop()
	{
		alSourceStop( m_iSource);
		alDeleteSources(1,&m_iSource);
		alGenSources(1,&m_iSource);
		ov_time_seek(&this->m_vorbis.m_vorbisFile,0);
		FillFirstCycle();
	}
	
	
	owAEBuffer* ow::owAEDevice::CreateBuffer()
	{
		owAEBuffer * pBuffer = new owAEBuffer;
		pBuffer->Init();
		return pBuffer;
	}
	
	owAEBuffer* owAEDevice::CreateBufferVorbis(ow::owBuffer* _pVorbisBuffer)
	{
		VorbisStruct vorbis;
		owINT32 error = ov_open_callbacks( _pVorbisBuffer, &vorbis.m_vorbisFile,NULL,0,OW_CALLBACKS_VORBIS);
		if( error != 0)
		{
			assert(false);
			return NULL;
		}
		vorbis.m_pBuffer = _pVorbisBuffer;
		// 读取信息与注释
		vorbis.m_pVorbisInfo = ov_info( &vorbis.m_vorbisFile, -1);
		vorbis.m_pVorbisComment = ov_comment( &vorbis.m_vorbisFile, -1);
		// 分析音频 format
		vorbis.m_nFrameBufferSize = vorbis.m_pVorbisInfo->rate * 2 * FRAME_BUFFER_TIME_MS / 1000.0f;
		switch(vorbis.m_pVorbisInfo->channels)
		{
			// 单声道
			case 1:
				vorbis.m_iFormat=AL_FORMAT_MONO16;
				break;
			// 双声道
			case 2:
				vorbis.m_iFormat=AL_FORMAT_STEREO16;
				vorbis.m_nFrameBufferSize*=2;
				break;
			// 四声道
			case 4:
				vorbis.m_iFormat=alGetEnumValue("AL_FORMAT_QUAD16");
				vorbis.m_nFrameBufferSize*=4;
				break;
			// 6声道
			case 6:
				vorbis.m_iFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				vorbis.m_nFrameBufferSize*=6;
				break;
			default:
				ov_clear(&vorbis.m_vorbisFile);
				return NULL;
		}
		vorbis.m_pFrameBuffer = new owCHAR[ vorbis.m_nFrameBufferSize];
		// 从vorbis里读取数据
		owINT32 pcmSize = ov_pcm_total(&vorbis.m_vorbisFile,-1);
		owBuffer * pPcmBuffer = CreateMemBuffer( pcmSize ); // 默认保留5s的大小，避免频繁的realloc
		while(vorbis.Eof() == owFALSE)
		{
			owINT32 decodeSize = vorbis.DecodeNextFrame();
			pPcmBuffer->Write(vorbis.m_pFrameBuffer, decodeSize);
		}
		owAEBuffer * pAEBuffer = new owAEBuffer;
		pAEBuffer->Init();
		pAEBuffer->BufferData(
					vorbis.m_iFormat,
					pPcmBuffer->GetBuffer(),
					pPcmBuffer->Size(),
					vorbis.m_pVorbisInfo->rate
					);
		return pAEBuffer;
	}

	owAEBuffer* owAEDevice::CreateBufferWav(ow::owBuffer* _pWavBuffer)
	{
		WAVE_INFO waveInfo;
		owBOOL ret = LoadWAVEAudio(_pWavBuffer, &waveInfo);
		if(ret == owFALSE)
		{
			return NULL;
		}
		owAEBuffer * pAEBuffer = new owAEBuffer;
		pAEBuffer->Init();
		pAEBuffer->BufferData(
				waveInfo.m_format,
				waveInfo.m_DataChunk.pData,
				waveInfo.m_DataChunk.nDataSize,
				waveInfo.m_samplePerSec
				);
		return pAEBuffer;
	}

	owAESource* owAEDevice::CreateSource()
	{
		owAESource * pSource = new owAESource();
		pSource->Init();
		return pSource;
	}

	owAEVorbisSource* owAEDevice::CreateVorbisSource( ow::owBuffer * _pVorbisBuffer )
	{
		owAEVorbisSource * pVorbisSource = new owAEVorbisSource();
		pVorbisSource->Init( _pVorbisBuffer);
		return pVorbisSource;
	}

	owBOOL owAEDevice::Init()
	{
		__CLEAR_AL_ERROR__
		
		if(m_pContext == NULL)
		{
			this->m_pDevice = alcOpenDevice( NULL);
			this->m_pContext = alcCreateContext( m_pDevice, NULL);
			alcMakeContextCurrent( m_pContext);
		}
		
		__CHECK_AL_ERROR__
		return owTRUE;
	}

	owVOID ow::owAEDevice::Release()
	{
		alcDestroyContext( m_pContext);
		alcCloseDevice( m_pDevice);
		delete this;
	}
	
	owAEDevice * __pDevice = NULL;
	
	owAEDevice * GetAudioDevice()
	{
		if(__pDevice == NULL)
		{
			__pDevice = new owAEDevice;
			__pDevice->Init();
		}
		return __pDevice;
	}

}


