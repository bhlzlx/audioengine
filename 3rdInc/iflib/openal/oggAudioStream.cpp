#include "AudioStream.h"
#include <stdint.h>
#include <iflib/Archive.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <assert.h>

namespace iflib
{
	namespace openal
	{
		size_t vorbisRead(void * _pData, size_t _nElement, size_t _nCount, void * _pFile)
		{
			IBlob* blob = (IBlob*)_pFile;
			return blob->Read(_pData, _nCount * _nElement);
		}

		int vorbisSeek(void * _pFile, ogg_int64_t _nOffset, int _flag)
		{
			IBlob* blob = (IBlob*)_pFile;
			return blob->Seek(_flag, (uint32_t)_nOffset);
		}

		int vorbisClose(void * _pFile)
		{
			IBlob* blob = (IBlob*)_pFile;
			blob->Release();
			return 1;
		}

		long vorbisTell(void * _pFile)
		{
			IBlob* blob = (IBlob*)_pFile;
			return blob->Tell();
		}

		static ov_callbacks IflibVorbisCallback = {
			(size_t(*)(void *, size_t, size_t, void *))  vorbisRead,
			(int(*)(void *, ogg_int64_t, int))           vorbisSeek,
			(int(*)(void *))                             vorbisClose,
			(long(*)(void *))                            vorbisTell
		};

		struct OggAudioStream : public IAudioStream
		{
			// 文件流
			IBlob * blob;
			// vorbis 文件句柄
			OggVorbis_File		oggvorbisFile;
			// vorbis信息
			vorbis_info*		vorbisInfo;
			// vorbis脚注
			vorbis_comment*		vorbisComment;
			//
			bool eof;
			// 通过 IAudioStream 继承
			OggAudioStream()
			{
				eof = false;
				vorbisInfo = nullptr;
				vorbisComment = nullptr;
			}

			virtual void Rewind() override
			{
				ov_time_seek(&oggvorbisFile, 0);
				eof = false;
			}

			virtual void Seek(double _time) override
			{
				ov_time_seek(&oggvorbisFile, _time);
				eof = false;
			}

			virtual float TimeCurr()
			{
				return ov_time_tell(&oggvorbisFile);
			}

			virtual float TimeTotal()
			{
				return ov_time_total(&oggvorbisFile, -1);
			}

			virtual size_t __ReadChunk(void * _pData) override
			{
				int size = 0;
				int result;
				int section;
				while (size < pcmFramesize)
				{
					result = ov_read(
						&this->oggvorbisFile,
						(char*)_pData + size,
						pcmFramesize - size,
						0,
						2,
						1,
						&section
					);
					// 文件尾
					if (result == 0)
					{
						eof = true;
						break;
					}
					// 正常读取
					else if (result > 0)
					{
						size = result + size;
					}
					// 发生错误
					else
					{
						assert(result != OV_HOLE && result != OV_EBADLINK && result != OV_EINVAL);
						break;
					}
				}
				return size;
			}

			virtual bool Eof() override
			{
				return eof;
			}

			virtual void Release() override
			{
				ov_clear(&oggvorbisFile);
				delete this;
			}
		};

		IAudioStream* IAudioStream::FromOGG(IBlob * _blob)
		{
			OggVorbis_File oggvorbisFile;
			int32_t error = ov_open_callbacks(_blob, &oggvorbisFile, NULL, 0, IflibVorbisCallback);
			if (error != 0)
			{
				return nullptr;
			}
			// 创建OggAudioStream对象
			OggAudioStream * audioStream = new OggAudioStream();
			audioStream->blob = _blob;
			audioStream->oggvorbisFile = oggvorbisFile;
			audioStream->vorbisInfo = ov_info(&audioStream->oggvorbisFile, -1);
			audioStream->vorbisComment = ov_comment(&audioStream->oggvorbisFile, -1);
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
			audioStream->bytesPerSec = audioStream->vorbisInfo->rate * 2;
			switch (audioStream->vorbisInfo->channels)
			{
				// 单声道
			case 1:
				audioStream->format = AL_FORMAT_MONO16;
				audioStream->channels = 1;
				//this->m_vorbis.m_nFrameBufferSize;
				break;
				// 双声道
			case 2:
				audioStream->format = AL_FORMAT_STEREO16;
				audioStream->channels = 2;
				audioStream->bytesPerSec *= 2;
				break;
				// 四声道
			case 4:
				audioStream->format = alGetEnumValue("AL_FORMAT_QUAD16");
				audioStream->channels = 4;
				audioStream->bytesPerSec *= 4;
				break;
				// 6声道
			case 6:
				audioStream->format = alGetEnumValue("AL_FORMAT_51CHN16");
				audioStream->channels = 6;
				audioStream->bytesPerSec *= 6;
				break;
			default:
				ov_clear(&audioStream->oggvorbisFile);
				delete audioStream;
				return nullptr;
			}
			audioStream->pcmFramesize = audioStream->bytesPerSec / 4;
			audioStream->pcmFramecount = audioStream->bytesPerSec / audioStream->pcmFramesize;
			audioStream->bitsPerSample = audioStream->vorbisInfo->bitrate_nominal;
			audioStream->frequency = audioStream->vorbisInfo->rate;
			audioStream->sampleDepth = audioStream->bytesPerSec / audioStream->frequency / audioStream->channels;
			//
			for (int i = 0; i < audioStream->pcmFramecount + 1; ++i)
			{
				PcmBuff pcmBuff;
				pcmBuff.data = new uint8_t[audioStream->pcmFramesize];
				pcmBuff.size = 0;
				audioStream->listPcmBuff.push_back(pcmBuff);
			}

			return audioStream;
		}
	}
}