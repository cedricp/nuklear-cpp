#ifndef TIMER_H
#define TIMER_H

#include <nuklear_lib.h>

/*
 * Timer execute callback (one-shot or periodically)
 * when time in milliseconds is reached
 */

class Timer : public NkEvent {
	unsigned long m_time, m_start_time;
	bool m_running, m_oneshot;
public:
	Timer(unsigned long time_ms, bool one_shot);
	virtual ~Timer();

	void start();
	void stop();

	bool is_active(){ return m_running;}
	unsigned long get_time(){return m_time;}
	unsigned long get_start_time(){return m_start_time;}

	virtual void on_timer_event();
};

#endif
