#ifndef PROCESS_H_
#define PROCESS_H_

#include <sys/types.h> // for pid_t
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <socket_notifier.h>

class Process;

class Process_manager {
	std::vector<Process*> m_processes;
	Process_manager(){

	}
public:
	static Process_manager* get();
	void add_process(Process* p);
	void remove_process(Process* p);
	static void child_handler(int signum);
};

class Process {
public:
	enum Communication_mode{
		NOCOMM = 0,
		STDIN = 1,
		STDOUT = 2,
		STDERR = 4,
		ALLOUTPUT = 6,
		ALL = 7,
		NOREAD
	};
	enum RunMode {
		DONTCARE,
		NOTIFYONEXIT,
		BLOCK
	};

	Process();
	virtual ~Process();

	bool run(std::string process_name, Communication_mode com_mode, const bool privileged = false);
	bool kill(int signo);
	void set_executable(std::string exec_name);
	void set_arguments(const std::vector<std::string>& args);
	void add_arguments(std::string arg);
	bool comm_setup_done_parent();
	bool comm_setup_done_child();
	void set_environment();
	pid_t get_child_pid();
	void child_exited(int status);
	bool dispatch_data(int which);

	bool is_running();

	virtual void on_stdin(char* buffer, int size);
	virtual void on_stdout(char* buffer, int size);
	virtual void on_stderr(char* buffer, int size);
	virtual void on_process_exit(int status);

	DECLARE_STATIC_CALLBACK_METHOD(on_data_available)
	DECLARE_CALLBACK_METHOD(on_data_available)

private:
	int  setup_communication(Communication_mode comm);
	void comm_close();
	pid_t m_pid;
	std::vector<std::string> m_arguments;
	bool m_runs;
	Communication_mode m_communication_mode;
	RunMode m_runmode;
	int m_out[2];
	int m_in[2];
	int m_err[2];

	Socket_notifier m_notifier;
	Socket_notifier m_stdout_notifier;
	Socket_notifier m_stderr_notifier;

};

#endif
