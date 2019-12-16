#include <thread.h>

IMPLEMENT_STATIC_CALLBACK_METHOD(on_exit_event, Thread)
typedef void * (*THREADFUNCPTR)(void *);

Thread::Thread(bool loop)  : m_running(false),
							m_pause(false),
							m_loop(loop),
							m_thread_id(0)
{
	CONNECT_CALLBACK((&m_thread_exit_event), on_exit_event)
}

Thread::~Thread()
{
	m_loop = false;
	join();
	m_running = false;
}

void Thread::start()
{
	if (!m_running){
		m_running = true;
		pthread_create(&m_thread_id, NULL, (THREADFUNCPTR)&Thread::run, this);
	}
}

void Thread::on_exit_event(NkEvent* sender_object, void* data)
{

}

void* Thread::run()
{
	while (m_running){
		if (!m_pause){
			entry();
		} else {
			usleep(10000U);
		}
		if (!m_loop)
			break;
	}
	m_running = false;
	m_thread_exit_event.push();
	pthread_exit(NULL);
}

void* Thread::join()
{
	if (m_thread_id){
		void* status = NULL;
		pthread_join(m_thread_id, &status);
		m_thread_id = 0;
		return status;
	}
	return NULL;
}

