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
			// �ļ���
			IBlob * blob;
			// vorbis �ļ����
			OggVorbis_File		oggvorbisFile;
			// vorbis��Ϣ
			vorbis_info*		vorbisInfo;
			// vorbis��ע
			vorbis_comment*		vorbisComment;
			//
			bool eof;
			// ͨ�� IAudioStream �̳�
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
					// �ļ�β
					if (result == 0)
					{
						eof = true;
						break;
					}
					// ������ȡ
					else if (result > 0)
					{
						size = result + size;
					}
					// ��������
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
			// ����OggAudioStream����
			OggAudioStream * audioStream = new OggAudioStream();
			audioStream->blob = _blob;
			audioStream->oggvorbisFile = oggvorbisFile;
			audioStream->vorbisInfo = ov_info(&audioStream->oggvorbisFile, -1);
			audioStream->vorbisComment = ov_comment(&audioStream->oggvorbisFile, -1);
			/******************************************************
			�������һ�´���
			PCM���ݴ�С���� ÿ���С = ������ * λ�� / 8 * ������
			λ����16,��������1��
			rate * 16 / 8 * 1
			�� rate * 2 ����һ��Ĵ�С
			250ms��Ӧ�Ĵ�СӦ����rate / 2
			�����������rate >> 1,λ��������һλ����ʵ�ֳ�2
			��������и�block����,������Ϊ�ǵ�����,16λ,�����ֽڵĶ���
			******************************************************/
			audioStream->bytesPerSec = audioStream->vorbisInfo->rate * 2;
			switch (audioStream->vorbisInfo->channels)
			{
				// ������
			case 1:
				audioStream->format = AL_FORMAT_MONO16;
				audioStream->channels = 1;
				//this->m_vorbis.m_nFrameBufferSize;
				break;
				// ˫����
			case 2:
				audioStream->format = AL_FORMAT_STEREO16;
				audioStream->channels = 2;
				audioStream->bytesPerSec *= 2;
				break;
				// ������
			case 4:
				audioStream->format = alGetEnumValue("AL_FORMAT_QUAD16");
				audioStream->channels = 4;
				audioStream->bytesPerSec *= 4;
				break;
				// 6����
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