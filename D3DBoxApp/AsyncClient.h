#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <deque>
#include <array>

using boost::asio::ip::tcp;

struct MoveTarget
{
	int x;
	int z;
};

class AsyncClient : public std::enable_shared_from_this<AsyncClient>
{
public:
	AsyncClient(boost::asio::io_context& io,
		const std::string& host,
		uint16_t port)
		: m_IO(io),
		m_Socket(io),
		m_EndPoint(boost::asio::ip::make_address(host), port)
	{
	}

	~AsyncClient()
	{
	}

	void Start();
	void Send(const std::string& msg);
	

	bool PopMoveTarget(MoveTarget& out);
	bool PopLine(std::string& out);

private:
	void DoRead();
	void DoWrite();
	void EnqueueLine(const std::string& line);
	void Parse(const char* data, size_t len);


private:
	boost::asio::io_context& m_IO;
	tcp::socket				 m_Socket;
	tcp::endpoint			 m_EndPoint;

	std::array<char, 512>    m_ReadBuf{};
	std::deque<std::string>  m_WriteQueue;

	std::string			     m_Incoming;
	std::mutex				 m_Mutex;
	std::queue<std::string>  m_Lines;
	std::queue<MoveTarget>   m_TargetQueue;
};

