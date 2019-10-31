#ifndef SOCKET_NOTIFIER_H
#define SOCKET_NOTIFIER_H

#include <vector>
#include <nuklear_lib.h>
#include <window_gles2.h>

class Socket_notifier : public NkEvent {
	std::vector<int> m_fds;
public:
	Socket_notifier(){
		NkWindow::get()->register_fd_event(this);
	}

	~Socket_notifier(){
		stop();
	}

	void add_fd(int fd){
		m_fds.push_back(fd);
	}

	void data_available(int which){
		if (m_callback)
			m_callback(this, (void*)which, m_callback_data);
	}

	const std::vector<int>& get_fds(){
		return m_fds;
	}

	void stop(){
		NkWindow::get()->unregister_fd_event(this);
		m_callback = NULL;
		m_callback_data = NULL;
	}
};


#endif
