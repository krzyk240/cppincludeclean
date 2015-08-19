#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG

# define D(...) __VA_ARGS__
# define E(...) eprintf(__VA_ARGS__)

#else

# define D(...)
# define E(...)

#endif

#define foreach(i,x) for (__typeof(x.begin()) i = x.begin(), \
	i ##__end = x.end(); i != i ##__end; ++i)

using std::vector;
using std::string;

int VERBOSITY = 1;
string compiler = getenv("CXX") ? getenv("CXX") : "";
string command;

const int GFBL_IGNORE_NEW_LINES = 1; // Erase '\n' from each line
/**
 * @brief Get file contents by lines in range [first, last)
 *
 * @param file filename
 * @param flags if set GFBL_IGNORE_NEW_LINES then '\n' is not appended to each
 * line
 * @param first number of first line to fetch
 * @param last number of first line not to fetch
 *
 * @return vector<string> containing fetched lines
 */
vector<string> getFileByLines(const char* file, int flags = 0,
	size_t first = 0, size_t last = -1) {
	vector<string> res;

	FILE *f = fopen(file, "r");
	if (f == NULL)
		return res;

	char *buff = NULL;
	size_t n = 0, line = 0;
	ssize_t read;

	while ((read = getline(&buff, &n, f)) != -1) {
		if ((flags & GFBL_IGNORE_NEW_LINES) && buff[read - 1] == '\n')
			buff[read - 1] = '\0';

		if (line >= first && line < last)
			res.push_back(buff);

		++line;
	}

	fclose(f);
	return res;
}

int putFileByLines(const char* file, const vector<string>& lines) {
	FILE *f = fopen(file, "w");
	if (f == NULL)
		return -1;

	size_t pos = 0, written;

	foreach(line, lines) {
		size_t line_pos = 0, len = line->size();
		while (line_pos < len) {
			written = fwrite(line->c_str() + line_pos, sizeof(char),
				len - line_pos, f);
			if (written == 0) {
				fclose(f);
				return pos;
			}

			line_pos += written;
			pos += written;
		}
	}

	fclose(f);
	return pos;
}

int putFileContents(const char* file, const char* data, size_t len = -1) {
	FILE *f = fopen(file, "w");
	if (f == NULL)
		return -1;

	if (len == size_t(-1))
		len = strlen(data);

	size_t pos = 0, written = 1;

	while (pos < len && written != 0) {
		written = fwrite(data + pos, sizeof(char), len - pos, f);
		pos += written;
	}

	fclose(f);
	return pos;
}

struct spawn_opts {
	int new_stdin_fd; // negative - close, STDIN_FILENO - do not change
	int new_stdout_fd; // negative - close, STDOUT_FILENO - do not change
	int new_stderr_fd; // negative - close, STDERR_FILENO - do not change
};

/*const spawn_opts default_spawn_opts = {
	STDIN_FILENO,
	STDOUT_FILENO,
	STDERR_FILENO
};*/

/**
 * @brief Runs exec using execvp(3) with arguments @p args
 *
 * @param exec file to execute
 * @param args arguments (last has to be NULL)
 * @param sopt spawn options - defines to what change stdin, stdout and stderr
 * negative field means to close stream
 *
 * @return exit code on success, -1 on error
 */
int spawn(const char* exec, const char* args[], const struct spawn_opts *opts) {
	pid_t cpid = fork();
	if (cpid == -1) {
		eprintf("%s:%i: %s: Failed to fork() - %s\n", __FILE__, __LINE__,
			__PRETTY_FUNCTION__, strerror(errno));
		return -1;

	} else if (cpid == 0) {
		// Change stdin
		if (opts->new_stdin_fd < 0)
			while (close(STDIN_FILENO) == -1 && errno == EINTR) {}

		else if (opts->new_stdin_fd != STDIN_FILENO)
			while (dup2(opts->new_stdin_fd, STDIN_FILENO) == -1)
				if (errno != EINTR)
					_exit(-1);

		// Change stdout
		if (opts->new_stdout_fd < 0)
			while (close(STDOUT_FILENO) == -1 && errno == EINTR) {}

		else if (opts->new_stdout_fd != STDOUT_FILENO)
			while (dup2(opts->new_stdout_fd, STDOUT_FILENO) == -1)
				if (errno != EINTR)
					_exit(-1);

		// Change stderr
		if (opts->new_stderr_fd < 0)
			while (close(STDERR_FILENO) == -1 && errno == EINTR) {}

		else if (opts->new_stderr_fd != STDERR_FILENO)
			while (dup2(opts->new_stderr_fd, STDERR_FILENO) == -1)
				if (errno != EINTR)
					_exit(-1);

		execvp(exec, (char *const *)args);
		_exit(-1);
	}

	int status;
	waitpid(cpid, &status, 0);
	return status;
};

/**
 * @brief Displays help
 */
static void help(const char* program_name) {
	if (program_name == NULL)
		program_name = "cppincludeclean";

	printf("Usage: %s [options] <path>...\n", program_name);
	printf("\
Comment out unnecessarily #include\n\
\n\
Options:\n\
  --compiler COMPILER\n\
                         Use COMPILER instead of $CXX\n\
  -c COMMAND, --command COMMAND\n\
                         Use shell COMMAND instead of compile command: COMPILER -c file\n\
  -h, --help             Display this information\n\
  -v, --verbose          Verbose mode\n");
}

/**
 * Pareses options passed to Conver via arguments
 * @param argc like in main (will be modified to hold number of no-arguments)
 * @param argv like in main (holds arguments)
 */
static void parseOptions(int &argc, char **argv) {
	int new_argc = 1;

	for (int i = 1; i < argc; ++i) {

		if (argv[i][0] == '-') {
			// Compiler
			if (0 == strcmp(argv[i], "--compiler") && i + 1 < argc)
				compiler = argv[++i];

			// Command
			if ((0 == strcmp(argv[i], "-c") ||
				0 == strcmp(argv[i], "--command")) && i + 1 < argc)
				command = argv[++i];

			// Help
			else if (0 == strcmp(argv[i], "-h") ||
					0 == strcmp(argv[i], "--help")) {
				help(argv[0]); // argv[0] is valid (argc > 1)
				exit(0);
			}

			// Verbose mode
			else if (0 == strcmp(argv[i], "-v") ||
					0 == strcmp(argv[i], "--verbose"))
				VERBOSITY = 2;

			// Unknown
			else
				eprintf("Unknown option: '%s'\n", argv[i]);

		} else
			argv[new_argc++] = argv[i];
	}

	argc = new_argc;
}

template<class T>
class UniquePtr {
private:
	UniquePtr(const UniquePtr&);
	UniquePtr& operator=(const UniquePtr&);

	T* p_;

public:
	explicit UniquePtr(T* ptr = NULL) : p_(ptr) {}

	~UniquePtr() {
		if (p_)
			delete p_;
	}

	void swap(UniquePtr<T>& up) {
		T* tmp = p_;
		p_ = up.p_;
		up.p_ = tmp;
	}

	T* get() const { return p_; }

	T* release(T* ptr = NULL) {
		T* tmp = p_;
		p_ = ptr;
		return tmp;
	}

	void reset(T* ptr = NULL) {
		if (p_)
			delete p_;
		p_ = ptr;
	}

	T& operator*() const { return *p_; }

	T* operator->() const { return p_; }

	T& operator[](size_t i) const { return p_[i]; }

	bool isNull() const { return !p_; }
};

template<class A>
inline bool operator==(const UniquePtr<A>& x, A* y) { return x.get() == y; }

template<class A>
inline bool operator==(A* x, const UniquePtr<A>& y) { return x == y.get(); }

template<class A, class B>
inline bool operator==(const UniquePtr<A>& x, const UniquePtr<B>& y) {
	return x.get() == y.get();
}

template<class A>
inline bool operator!=(const UniquePtr<A>& x, A* y) { return x.get() != y; }

template<class A>
inline bool operator!=(A* x, const UniquePtr<A>& y) { return x != y.get(); }

template<class A, class B>
inline bool operator!=(const UniquePtr<A>& x, const UniquePtr<B>& y) {
	return x.get() != y.get();
}

class File {
	vector<string> file;
	vector<string>::iterator line_;
	string fname, tmp;
	bool ok; /* true - file is in appropriate version,
				false - testing with commented #include */

public:
	File(const char* filename) : file(getFileByLines(filename)),
		line_(file.begin()), fname(filename), tmp(), ok(true) {}

	vector<string>::iterator begin() { return file.begin(); }

	vector<string>::iterator end() { return file.end(); }

	vector<string>::iterator line() { return line_; }

	vector<string>::iterator next() {
		ok = true;
		return ++line_;
	}

	void toggleCommentOnLine() {
		if (ok) {
			tmp = "// " + *line_;
			line_->swap(tmp);
			ok = false;
			writeOut();

		} else {
			line_->swap(tmp);
			ok = true;
		}
	}

	void writeOut() { putFileByLines(fname.c_str(), file); }

	~File() {
		if (!ok)
			line_->swap(tmp);
		// Ensure that file is in appropriate version
		writeOut();
	}
};

UniquePtr<File> file;

int main(int argc, char *argv[]) {
	parseOptions(argc, argv);

	if (argc < 2) {
		help(argc > 0 ? argv[0] : NULL);
		return 0;
	}

	// Install signal handlers
	struct sigaction sa;
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = &exit;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	int fd = open("/dev/null", O_RDWR | O_LARGEFILE);
	if (fd < 0) {
		eprintf("Error: open(\"/dev/null\") - %s", strerror(errno));
		return 1;
	}

	spawn_opts run_opts = { -1, fd, fd };

	for (int i = 1; i < argc; ++i) {
		file.reset(new File(argv[i]));
		for (; file->line() != file->end(); file->next()) {
			// Check if file->line() is #include
			string::iterator it = file->line()->begin();
			// Remove leading white spaces
			while (it != file->line()->end() && isspace(*it))
				++it;

			if (*it++ != '#')
				continue;

			// Remove leading white spaces
			while (it != file->line()->end() && isspace(*it))
				++it;

			if (!equal(it, min(it + 7, file->line()->end()), "include"))
				continue;

			if (VERBOSITY > 1)
				printf("%s:%zu: %s", argv[i], file->line() - file->begin(),
					file->line()->c_str());

			// Try to compile without #include
			file->toggleCommentOnLine();

			const char* args[] = {
				compiler.c_str(),
				"-c",
				argv[i],
				NULL
			};

			if (command.size()) {
				args[0] = "sh";
				args[2] = command.c_str();
			}

			// Run
			if (spawn(args[0], args, &run_opts) != 0)
				file->toggleCommentOnLine(); // Uncomment
		}
	}

	return 0;
}
