#include "D3DBoxServer.h"
#include <exception>

int main()
{
	try
	{
		boost::asio::io_context io;
		tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));

		std::cout << "Server started on port 8080\n";
		DoAccept(acceptor);

		io.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Server exception: " << e.what() << "\n";
	}

	return 0;
}