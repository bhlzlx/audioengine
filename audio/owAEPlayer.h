#ifndef __OW_AEPLAYER_H__
#define __OW_AEPLAYER_H__

#include "owAudio.h"
#include <pthread.h>
#include <queue>

namespace ow
{

	using namespace std;

	typedef enum
	{
		eAEPlay,
		eAEPause,
		eAEStop,
		eAEDestroy,
		eAEIdle
	} MSGType;

	struct AEMSG
	{
		MSGType type;
	};

	struct AEMessageQueue;



	struct owAEMusicPlayer
	{
	private:
		ow::owAEVorbisSource * 	m_pMusicSrc;
		pthread_t	    		m_thread_t;
		AEMessageQueue * 		m_pMsgQueue;
		AEMSG			 		m_state;
		
		pthread_cond_t 			m_waitCond;
		pthread_mutex_t 		m_waitMutex;


	private:
		void __Release();
		void __Play();
		void __Pause();
		void __Stop();

		
	public:
		owAEMusicPlayer(void);
		~owAEMusicPlayer(void);
		bool Init( ow::owBuffer * _pFileBuffer);

		void Release();
		void Play();
		void Pause();
		void Stop();
		
		static void * MusicThreadProc(void *);
	};



	struct AEMessageQueue
	{
		std::queue<AEMSG> m_msg_queue;
		pthread_mutex_t   m_msg_mutex;

		AEMSG			  m_state;

		AEMessageQueue();

		void Release();

		void PostMsg(AEMSG & msg);
		bool GetMsg(AEMSG &msg);
	};

}

#endif // __OW_AEPLAYER_H__
