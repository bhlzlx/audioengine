#ifndef __OW_AUDIO_H__
#define __OW_AUDIO_H__

/************************************************************************
 *  OpenAL - libogg - libvorbis 相关代码，由于匆忙，没有定义接口（用于扩展其它格式）
 *  直接写下定义
 ***********************************************************************/

#include <al/al.h>
#include <al/alc.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <ogg/ogg.h>
#include <owcmn/owcmn.h>
#include <assert.h>
#include <owfile/owfile.h>

#define __CLEAR_AL_ERROR__ \
	alGetError();
#define __CHECK_AL_ERROR__ \
	assert ( alGetError() == AL_NO_ERROR );
	
typedef void (* AL_FLOAT_SETTER )(ALuint, ALenum, ALfloat);
typedef void (* AL_FLOAT_GETTER )(ALuint, ALenum, ALfloat *);
typedef void (* AL_INT32_SETTER )(ALuint, ALenum, ALint);
typedef void (* AL_INT32_GETTER )(ALuint, ALenum, ALint *);
typedef void (* AL_INT32_VEC_SETTER )(ALuint, ALenum, const ALint *);
typedef void (* AL_FLOAT_VEC_SETTER )(ALuint, ALenum, const ALfloat *);

	
#define AL_SOURCE_GET_SET_DEF( TYPE, PARAM ) \
	TYPE Get_##PARAM(); \
	owVOID Set_##PARAM( TYPE );

#define AL_SOURCE_GET_SET_IMP( SPACE , TYPE, PARAM )\
	TYPE SPACE::Get_##PARAM(){\
		static TYPE value;\
		owAESourceGetter_##TYPE( m_iSource, AL_##PARAM, &value);\
		return value;\
	}\
	owVOID SPACE::Set_##PARAM( TYPE value){\
		owAESourceSetter_##TYPE(m_iSource, AL_##PARAM, value);\
	}\
	
#define AL_SOURCE_GET_SET_VEC_DEF( TYPE, PARAM )\
	owVOID Get_##PARAM( TYPE * );\
	owVOID Set_##PARAM( TYPE * );

#define AL_SOURCE_GET_SET_VEC_IMP( SPACE , TYPE, PARAM )\
	owVOID SPACE::Get_##PARAM( TYPE * _pValue){\
		owAESourceSetter_vec_##TYPE( m_iSource, AL_##PARAM, _pValue);\
	}\
	owVOID SPACE::Set_##PARAM( TYPE * _pValue){\
		owAESourceGetter_vec_##TYPE( m_iSource, AL_##PARAM, _pValue);\
	}\

namespace ow
{	
	struct owAEBuffer
	{
		owUINT32		m_iBuffer;
		owINT32 		m_nDepthBit;
		owINT32 		m_nChannels;
		owINT32 		m_nSize;
		owINT32 		m_nFrequency;
		
		owBOOL Init();
		owVOID BufferData( 
					owINT32 _format, 
					const owVOID * _pData, 
					owINT32 _size, 
					owINT32 _freq 
					);
		owBOOL Valid();
		
		owINT32 GetFrequency();
		owINT32 GetDepthBits();
		owINT32 GetChannels();
		owINT32 GetSize();
		owVOID Release();
		~owAEBuffer();
	};	
	
	struct owAESource
	{
		owUINT32		m_iSource;
		owAEBuffer*		m_pBuffer;
		
		owBOOL Init();
		
		owBOOL Valid();
		owVOID Play();
		owVOID Stop();
		owVOID Pause();
		owVOID SetBuffer( owAEBuffer * _pBuffer );
		
		owVOID SetFloat( float* _pValue, owINT32 _attr);
		owVOID SetInt( int* _pValue, owINT32 _attr);
		owVOID GetFloat( float * _pValue, owINT32 _attr);
		owVOID GetInt( int * _pValue, owINT32 _attr);
		owVOID Release();
		
		AL_SOURCE_GET_SET_DEF( float, PITCH)
		AL_SOURCE_GET_SET_DEF( float, GAIN)
		AL_SOURCE_GET_SET_DEF( float, MAX_DISTANCE)
		AL_SOURCE_GET_SET_DEF( float, ROLLOFF_FACTOR)
		AL_SOURCE_GET_SET_DEF( float, REFERENCE_DISTANCE)
		AL_SOURCE_GET_SET_DEF( float, MIN_GAIN)
		AL_SOURCE_GET_SET_DEF( float, MAX_GAIN)
		AL_SOURCE_GET_SET_DEF( float, CONE_OUTER_GAIN)
		AL_SOURCE_GET_SET_DEF( float, CONE_INNER_ANGLE)
		AL_SOURCE_GET_SET_DEF( float, CONE_OUTER_ANGLE)
		AL_SOURCE_GET_SET_DEF( float, SEC_OFFSET)
		AL_SOURCE_GET_SET_DEF( float, BYTE_OFFSET)
		
		AL_SOURCE_GET_SET_DEF( int, SOURCE_RELATIVE)
		AL_SOURCE_GET_SET_DEF( int, SOURCE_TYPE);
		AL_SOURCE_GET_SET_DEF( int, LOOPING);
		AL_SOURCE_GET_SET_DEF( int, BUFFER);
		AL_SOURCE_GET_SET_DEF( int, SOURCE_STATE);
		AL_SOURCE_GET_SET_DEF( int, BUFFERS_QUEUED);
		AL_SOURCE_GET_SET_DEF( int, BUFFERS_PROCESSED);
		
		AL_SOURCE_GET_SET_VEC_DEF( float, POSITION);
		AL_SOURCE_GET_SET_VEC_DEF( float, VELOCITY);
		AL_SOURCE_GET_SET_VEC_DEF( float, DIRECTION);
	};
	
	struct VorbisStruct
	{
		VorbisStruct()
		{
			m_pFrameBuffer = NULL;
		}
		~VorbisStruct()
		{
			if(m_pFrameBuffer)
			{
				delete []m_pFrameBuffer;
			}
		}
		// vorbis数据源
		ow::owBuffer*		m_pBuffer;
		// vorbis对象
		OggVorbis_File		m_vorbisFile;
		// vorbis文件信息
		vorbis_info*		m_pVorbisInfo;
		// vorbis脚注
	    vorbis_comment*		m_pVorbisComment;
		// AL 格式
		owINT32				m_iFormat;
		// PCM 解码数据缓冲区
		owCHAR*				m_pFrameBuffer;
		owINT32				m_nFrameBufferSize;
		
		owINT32 DecodeNextFrame();
		owINT32 Size();
		owINT32 Tell();
		owBOOL Eof();
		owVOID Clear();
	};
	
	struct owAEVorbisSource
	{
		owUINT32		m_vecBuffers[3];	// 三个buffer轮循
		owUINT32		m_iSource;			// source 对象
		VorbisStruct	m_vorbis;
		
		owVOID Init( owBuffer * _pBuffer);
		owBOOL UpdateBuffer();
		owVOID Play();
		owVOID Stop();
		owVOID Pause();
		owVOID Release();
	private:
		owVOID FillFirstCycle();
	};
	
	struct owAEDevice
	{
		ALCdevice *			m_pDevice;
		ALCcontext *	    m_pContext;
		
		owAEDevice()
		{
			m_pDevice = NULL;
			m_pContext = NULL;
		}
		
		owBOOL Init();
		owVOID Release();
		
		owAEBuffer * CreateBuffer();
		/**
		 * @brief : Create wav & ogg buffer for sound effects
		 */
		owAEBuffer * CreateBufferWav( ow::owBuffer * _pWavBuffer);
		owAEBuffer * CreateBufferVorbis( ow::owBuffer * _pVorbisBuffer);
		
		owAESource * CreateSource();
		/**
		 * @brief : function for creating a source for music playing!
		 * @param : _pVorbisBuffer : an ogg file buffer
		 */
		owAEVorbisSource * CreateVorbisSource( ow::owBuffer * _pVorbisBuffer );
	};
	
	owAEDevice * GetAudioDevice();
};

#endif