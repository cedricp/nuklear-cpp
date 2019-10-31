#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <unistd.h>
#include <nuklear_lib.h>

class Thread_mutex {
	pthread_mutex_t m_mutex;
public:
	Thread_mutex(){
		pthread_mutex_init(&m_mutex, NULL);
	}
	~Thread_mutex(){
		pthread_mutex_destroy(&m_mutex);
	}
	bool try_lock(){
		return pthread_mutex_trylock(&m_mutex) == 0;
	}
	bool lock(){
		return pthread_mutex_lock(&m_mutex) == 0;
	}
	bool unlock(){
		return pthread_mutex_unlock(&m_mutex) == 0;
	}
};

class Thread_auto_mutex
{
	Thread_mutex& m_mutex;
public:
	Thread_auto_mutex(Thread_mutex& mutex) : m_mutex(mutex){
		m_mutex.lock();
	}
	~Thread_auto_mutex(){
		m_mutex.unlock();
	}
};

class Thread {
	UserEvent m_thread_exit_event;
	pthread_t m_thread_id;
	volatile bool m_running;
	volatile bool m_pause;
	bool m_loop;

	void* run();
	DECLARE_STATIC_CALLBACK_METHOD(on_exit_event)
public:
	Thread(bool loop);
	virtual ~Thread();
	void start();
	void* join();

	/*
	 * Method must be overriden if thread exit event is needed
	 * It's executed in the main thread
	 */
	virtual void on_exit_event(NkEvent* sender_object, void* data);

	/*
	 * Main thread code
	 * If the code needs to loop, set loop=true in the constructor
	 */
	virtual void entry() = 0;

	void pause(bool p){
		m_pause = p;
	}

	void stop(){
		m_running = false;
	}

	bool is_paused(){
		return m_pause;
	}

	bool is_running(){
		return m_running;
	}
};

#endif
