#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;

class AsyncClient : public std::enable_shared_from_this<AsyncClient>
{
public:
	AsyncClient(boost::asio::io_context& io, const std::string& host, uint16_t port)
		: m_IO(io)
		, m_Socket(io)
		, m_Endpoint(boost::asio::ip::make_address(host), port)
	{
	}

	void Start()
	{
		DoConnect();
	}

	void Send(const std::string& msg)
	{
		// 메인스레드에서 호출해도 안전하게 write 가능하도록 strand 권장
		auto self = shared_from_this();
		boost::asio::post(m_IO,
			[this, self, msg]()
			{
				bool writing = !m_WriteQueue.empty();
				m_WriteQueue.push_back(msg);

				if (!writing)
					DoWrite();
			});
	}

private:
	// -----------------------------
	// 1. Connect
	// -----------------------------
	void DoConnect()
	{
		auto self = shared_from_this();
		m_Socket.async_connect(m_Endpoint,
			[this, self](boost::system::error_code ec)
			{
				if (!ec)
				{
					std::cout << "Connected to server.\n";
					DoRead();
				}
				else
				{
					std::cout << "Connect failed: " << ec.message() << "\n";
				}
			});
	}

	// -----------------------------
	// 2. Read (Echo Server Response)
	// -----------------------------
	void DoRead()
	{
		auto self = shared_from_this();
		m_Socket.async_read_some(boost::asio::buffer(m_ReadBuf),
			[this, self](boost::system::error_code ec, std::size_t len)
			{
				if (!ec)
				{
					std::string msg(m_ReadBuf.data(), len);
					std::cout << "Server: " << msg << "\n";

					DoRead(); // 계속 읽기
				}
				else
				{
					std::cout << "Read error: " << ec.message() << "\n";
				}
			});
	}

	// -----------------------------
	// 3. Write
	// -----------------------------
	void DoWrite()
	{
		auto self = shared_from_this();
		boost::asio::async_write(m_Socket,
			boost::asio::buffer(m_WriteQueue.front()),
			[this, self](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					m_WriteQueue.pop_front();
					if (!m_WriteQueue.empty())
						DoWrite();
				}
				else
				{
					std::cout << "Write error: " << ec.message() << "\n";
				}
			});
	}

private:
	boost::asio::io_context& m_IO;
	tcp::socket m_Socket;
	tcp::endpoint m_Endpoint;
	//위의 3개는 고정으로 정해져있음

	std::array<char, 1024> m_ReadBuf;

	std::deque<std::string> m_WriteQueue;
};

int main()
{
	boost::asio::io_context io;

	auto client = std::make_shared<AsyncClient>(io, "127.0.0.1", 7777);
	client->Start();

	// 입력을 보내는 스레드는 따로
	std::thread input_thread([client]()
		{
			while (true)
			{
				std::string line;
				std::getline(std::cin, line);
				if (line == "quit") break;

				client->Send(line);
			}
		});

	io.run();
	input_thread.join();

	return 0;
}
