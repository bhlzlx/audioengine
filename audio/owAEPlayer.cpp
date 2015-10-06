#include "owAEPlayer.h"

namespace ow
{
	#include <time.h>
	#ifdef WIN32
	#   include <windows.h>
	#else
	#   include <sys/time.h>
	#endif
	#ifdef WIN32
	int
	gettimeofday(struct timeval *tp, void *tzp)
	{
		time_t clock;
		struct tm tm;
		SYSTEMTIME wtm;
		GetLocalTime(&wtm);
		tm.tm_year     = wtm.wYear - 1900;
		tm.tm_mon     = wtm.wMonth - 1;
		tm.tm_mday     = wtm.wDay;
		tm.tm_hour     = wtm.wHour;
		tm.tm_min     = wtm.wMinute;
		tm.tm_sec     = wtm.wSecond;
		tm. tm_isdst    = -1;
		clock = mktime(&tm);
		tp->tv_sec = clock;
		tp->tv_usec = wtm.wMilliseconds * 1000;
		return (0);
	}
	#endif
 
	
	AEMessageQueue::AEMessageQueue()
	{
		pthread_mutex_init(&m_msg_mutex,NULL);
	}

	void AEMessageQueue::Release()
	{
		pthread_mutex_destroy(&m_msg_mutex);
	}

	void AEMessageQueue::PostMsg(AEMSG & msg)
	{
		pthread_mutex_lock(&m_msg_mutex);
		m_msg_queue.push(msg);
		pthread_mutex_unlock(&m_msg_mutex);
	}

	bool AEMessageQueue::GetMsg(AEMSG &msg)
	{
		pthread_mutex_lock(&m_msg_mutex);
		if(m_msg_queue.size() <= 0)
		{
			pthread_mutex_unlock(&m_msg_mutex);
			return false;
		}
		msg = m_msg_queue.front();
		m_msg_queue.pop();
		pthread_mutex_unlock(&m_msg_mutex);
		return true;
	}


	owAEMusicPlayer::owAEMusicPlayer(void)
	{

	}


	owAEMusicPlayer::~owAEMusicPlayer(void)
	{

	}

	bool owAEMusicPlayer::Init( ow::owBuffer * _pFileBuffer)
	{
		m_pMusicSrc = GetAudioDevice()->CreateVorbisSource( _pFileBuffer);
		m_pMsgQueue = new AEMessageQueue();
		m_state.type=eAEIdle;
		pthread_create(&this->m_thread_t,NULL,owAEMusicPlayer::MusicThreadProc,this);
		
		pthread_mutex_init( &this->m_waitMutex, NULL);
		pthread_cond_init( &this->m_waitCond, NULL);

		return true;
	}

	void owAEMusicPlayer::Release()
	{
		AEMSG aemsg;
		aemsg.type=eAEDestroy;
		m_pMsgQueue->PostMsg(aemsg);
	}
	void owAEMusicPlayer::Play()
	{
		AEMSG aemsg;
		aemsg.type=eAEPlay;
		m_pMsgQueue->PostMsg(aemsg);
	}
	void owAEMusicPlayer::Pause()
	{
		AEMSG aemsg;
		aemsg.type=eAEPause;
		m_pMsgQueue->PostMsg(aemsg);
	}
	void owAEMusicPlayer::Stop()
	{
		AEMSG aemsg;
		aemsg.type=eAEStop;
		m_pMsgQueue->PostMsg(aemsg);
	}

	void owAEMusicPlayer::__Release()
	{
		this->m_pMusicSrc->Release();
		this->m_pMsgQueue->Release();
		delete this;
	}

	void owAEMusicPlayer::__Play()
	{
		if(m_state.type != eAEPlay)
		{
			m_pMusicSrc->Play();
			m_state.type=eAEPlay;
		}
		
	}

	void owAEMusicPlayer::__Pause()
	{
		if(m_state.type==eAEPlay)
		{
			m_pMusicSrc->Pause();
			m_state.type=eAEPause;
		}
		
	}
	void owAEMusicPlayer::__Stop()
	{
		if(m_state.type==eAEPlay)
		{
			m_pMusicSrc->Stop();
			m_state.type=eAEPause;
		}
	}

	void * owAEMusicPlayer::MusicThreadProc(void * _ptr)
	{
		owAEMusicPlayer * pPlayer = (owAEMusicPlayer *)_ptr;
		while(true)
		{
			AEMSG msg;
			bool r = pPlayer->m_pMsgQueue->GetMsg(msg);
			if(r)
			{
				switch(msg.type)
				{
				case eAEPlay:
					  pPlayer->__Play();
					break;
				case eAEPause:
					pPlayer->__Pause();
					break;
				case eAEStop:
					pPlayer->__Stop();
					break;
				case eAEDestroy:
					pPlayer->__Release();
					return NULL;
				}
			}
			else
			{
				if(pPlayer->m_state.type == eAEPlay)
				{
					pPlayer->m_pMusicSrc->UpdateBuffer();
				}
			}
			// 避免栈中频繁分配 使用static关键字，提高效率
			static struct timeval now;
			static struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec;
			outtime.tv_nsec = (now.tv_usec + 400 * 1000) * 1000; // 400 毫秒
			pthread_cond_timedwait(&pPlayer->m_waitCond, &pPlayer->m_waitMutex, &outtime);
		}
	}
}

