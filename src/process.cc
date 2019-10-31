#include <process.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <algorithm>

Process::Process() {
	m_pid = 0;
	m_out[0] = m_out[1] = -1;
	m_in[0] = m_in[1] = -1;
	m_err[0] = m_err[1] = -1;
}

Process::~Process() {
	comm_close();
	this->kill(SIGKILL);
	Process_manager::get()->remove_process(this);
}

bool Process::run(std::string process_name, Communication_mode com_mode, const bool run_privileged) {
	m_arguments.insert(m_arguments.begin(), process_name);

	std::vector<const char*> args;
	for (auto& arg : m_arguments) {
		args.push_back(arg.c_str());
	}
	args.push_back(NULL);

	if (!setup_communication(com_mode)){

	}

	uid_t uid = getuid();
	gid_t gid = getgid();

	int fd[2];
	if (0 > pipe(fd)) {
		fd[0] = fd[1] = 0; // Pipe failed.. continue
	}

	m_pid = fork();

	if (0 == m_pid) {
		if (fd[0])
			close(fd[0]);
		if (!run_privileged) {
			setgid(gid);
			setuid(uid);
		}

		if (!comm_setup_done_child()){

		}

		set_environment();

		if (0){
			// Don't care mode
			setpgid(0,0);
		}

		struct sigaction act;
		sigemptyset(&(act.sa_mask));
		sigaddset(&(act.sa_mask), SIGPIPE);
		act.sa_handler = SIG_DFL;
		act.sa_flags = 0;
		sigaction(SIGPIPE, &act, 0L);

		if (fd[1])
			fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		execvp(args[0], (char* const*)&args[0]);
		char resultByte = 1;
		if (fd[1])
			write(fd[1], &resultByte, 1);
		_exit(-1);
	} else if (-1 == m_pid) {
		// forking failed

		m_runs = false;
		return false;
	} else {
		Process_manager::get()->add_process(this);
		if (fd[1])
			close(fd[1]);

		if (fd[0]){
			for (;;) {
				char resultByte;
				int n = ::read(fd[0], &resultByte, 1);
				if (n == 1) {
					// Error
					m_runs = false;
					close(fd[0]);
					m_pid = 0;
					return false;
				}
				if (n == -1) {
					if ((errno == ECHILD) || (errno == EINTR))
						continue; // Ignore
				}
				break; // success
			}
		}
		if (fd[0]){
			close(fd[0]);
		}

		if (!comm_setup_done_parent()){

		}
		// TODO : Add blocking mode
	}
	return true;
}

bool
Process::comm_setup_done_child()
{
	bool ok = true;
	struct linger so;
	memset((void*)&so, 0, sizeof(so));

	if (m_communication_mode & STDIN)
		close (m_in[1]);
	if (m_communication_mode & STDOUT)
		close (m_out[0]);
	if (m_communication_mode & STDERR)
		close (m_err[0]);

	if (m_communication_mode & STDIN)
		ok &= dup2(m_in[0], STDIN_FILENO) != -1;
	else {
		int null_fd = open("/dev/null", O_RDONLY);
		ok &= dup2(null_fd, STDIN_FILENO) != -1;
		close(null_fd);
	}
	if (m_communication_mode & STDOUT) {
		ok &= dup2(m_out[1], STDOUT_FILENO) != -1;
		ok &= !setsockopt(m_out[1], SOL_SOCKET, SO_LINGER, (char*) &so,
				sizeof(so));
	} else {
		int null_fd = open("/dev/null", O_WRONLY);
		ok &= dup2(null_fd, STDOUT_FILENO) != -1;
		close(null_fd);
	}
	if (m_communication_mode & STDERR) {
		ok &= dup2(m_err[1], STDERR_FILENO) != -1;
		ok &= !setsockopt(m_err[1], SOL_SOCKET, SO_LINGER,
				reinterpret_cast<char *>(&so), sizeof(so));
	} else {
		int null_fd = open("/dev/null", O_WRONLY);
		ok &= dup2(null_fd, STDERR_FILENO) != -1;
		close(null_fd);
	}
	return ok;
}

void Process::set_executable(std::string exec_name) {
	if (exec_name.empty())
		return;
	m_arguments.insert(m_arguments.begin(), exec_name);
}

bool Process::kill(int signo) {
	if (m_pid != 0) {
		int ret = ::kill(m_pid, signo);
		if (ret == -1)
			return false;
		return true;
	}
	return false;
}

void
Process::set_arguments(const std::vector<std::string>& args) {
	if (args.empty())
		return;
	m_arguments.insert(m_arguments.end(), args.begin(), args.end());
}

void Process::add_arguments(std::string arg)
{
	m_arguments.push_back(arg);
}

bool
Process::comm_setup_done_parent()
{
	bool ok = true;

	if (m_communication_mode != NOCOMM) {
		if (m_communication_mode & STDIN)
			close(m_in[0]);
		if (m_communication_mode & STDOUT)
			close(m_out[1]);
		if (m_communication_mode & STDERR)
			close(m_err[1]);

		// Don't create socket notifiers and set the sockets non-blocking if
		// blocking is requested.
		if (m_runmode == BLOCK)
			return ok;

		if (m_communication_mode & STDIN) {
			m_notifier.add_fd(m_in[1]);
		}

		if (m_communication_mode & STDOUT) {
			m_notifier.add_fd(m_out[0]);
		}

		if (m_communication_mode & STDERR) {
			m_notifier.add_fd(m_err[0]);
		}

		CONNECT_CALLBACK2((&m_notifier), on_data_available, this);
	}
	return ok;
}

int Process::setup_communication(Communication_mode comm) {
	int ok;

	m_communication_mode = comm;

	ok = 1;
	if (comm & STDIN){
		ok &= socketpair(AF_UNIX, SOCK_STREAM, 0, m_in) >= 0;
	}
	if (comm & STDOUT){
		ok &= socketpair(AF_UNIX, SOCK_STREAM, 0, m_out) >= 0;
	}

	if (comm & STDERR){
		ok &= socketpair(AF_UNIX, SOCK_STREAM, 0, m_err) >= 0;
	}
	return ok;
}

void Process::set_environment() {
	// TODO
}

void
Process::comm_close() {
	if (NOCOMM != m_communication_mode) {
		bool b_in = (m_communication_mode & STDIN);
		bool b_out = (m_communication_mode & STDOUT);
		bool b_err = (m_communication_mode & STDERR);

		if (m_communication_mode & STDIN) {
			m_communication_mode = (Communication_mode)(m_communication_mode & ~STDIN);
			close (m_in[1]);
		}
		if (m_communication_mode & STDOUT) {
			m_communication_mode = (Communication_mode)(m_communication_mode & ~STDOUT);
			close (m_out[0]);
		}
		if (m_communication_mode & STDERR) {
			m_communication_mode = (Communication_mode)(m_communication_mode & ~STDERR);
			close (m_err[0]);
		}
	}
}

void
Process::on_stdin(char* buffer, int size)
{
	printf("in> %s\n", buffer);
}

void
Process::on_stdout(char* buffer, int size)
{
	printf("out> %s\n", buffer);
}

void
Process::on_stderr(char* buffer, int size)
{
	printf("err>% s\n", buffer);
}

void
Process::on_process_exit(int status) {
	printf("Exited %i\n", status);
}


IMPLEMENT_CALLBACK_METHOD(on_data_available, Process) {
	int which = (long)data;
	dispatch_data(which);
}

bool Process::dispatch_data(int which) {
	char buffer[1024];
	bool buffer_empty = true;
	if (which == m_out[0]){
		int size = ::read(m_out[0], buffer, 1024);
		if (size){
			on_stdout(buffer, size);
			buffer_empty = false;
		}
	}
	if (which == m_err[0]){
		int size = ::read(m_err[0], buffer, 1024);
		if (size){
			on_stderr(buffer, size);
			buffer_empty = false;
		}
	}
	if (which == m_in[1]){
		int size = ::read(m_in[1], buffer, 1024);
		if (size){
			on_stdin(buffer, size);
			buffer_empty = false;
		}
	}
	return buffer_empty;
}


bool Process::is_running() {
	return m_runs;
}

pid_t Process::get_child_pid() {
	return m_pid;
}

void Process::child_exited(int status) {
	m_notifier.stop();
	// Flush remaining data
	while(!dispatch_data(m_out[0])){}
	while(!dispatch_data(m_err[0])){}
	while(!dispatch_data(m_in[1])){}
	comm_close();
	on_process_exit(status);
}

static Process_manager* global_pm = NULL;

Process_manager* Process_manager::get() {
	if (global_pm == 0){
		global_pm = new Process_manager;
		signal(SIGCHLD, global_pm->child_handler);
	}
	return global_pm;
}

void Process_manager::add_process(Process* p) {
	if (std::find(m_processes.begin(), m_processes.end(), p) == m_processes.end()){
		m_processes.push_back(p);
	}
}

void Process_manager::remove_process(Process* p) {
	std::vector<Process*>::iterator it = std::find(m_processes.begin(), m_processes.end(), p);
	if (it != m_processes.end()){
		m_processes.erase(it);
	}
}

void Process_manager::child_handler(int signum) {
	pid_t childpid;
	int childstatus;
	childpid = waitpid( -1, &childstatus, WNOHANG);

	for (Process* p : global_pm->m_processes){
		if (p->get_child_pid() == childpid){
			p->child_exited(childstatus);
		}
	}
}

