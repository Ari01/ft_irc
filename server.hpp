#include <vector>

class Server
{
	private:
		std::vector<struct pollfd>	pfds;
		int							listener_fd;
		struct pollfd				new_pfd(int fd, short events, short revents);
		
	public:
		Server();
		~Server();

		void	run();
};
