#include "AsyncClient.h"

void AsyncClient::Start()
{
	auto self = shared_from_this();
	m_Socket.async_connect(m_EndPoint,
		[this, self](boost::system::error_code ec)
		{
			if (!ec)
				DoRead();
		});
}

void AsyncClient::Send(const std::string& msg)
{
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

bool AsyncClient::PopMoveTarget(MoveTarget& out)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_TargetQueue.empty())
		return false;

	out = m_TargetQueue.front();
	m_TargetQueue.pop();
	return true;
}

bool AsyncClient::PopLine(std::string& out)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Lines.empty())
		return false;

	out = m_Lines.front();
	m_Lines.pop();
	return true;
}

void AsyncClient::DoRead()
{
	auto self = shared_from_this();
	m_Socket.async_read_some(
		boost::asio::buffer(m_ReadBuf),
		[this, self](boost::system::error_code ec, std::size_t len)
		{
			if (!ec)
			{
				Parse(m_ReadBuf.data(), len);
				DoRead();
			}
		});
}

void AsyncClient::DoWrite()
{
	auto self = shared_from_this();
	boost::asio::async_write(
		m_Socket,
		boost::asio::buffer(m_WriteQueue.front()),
		[this, self](boost::system::error_code ec, std::size_t)
		{
			if (!ec)
			{
				m_WriteQueue.pop_front();
				if (!m_WriteQueue.empty())
					DoWrite();
			}
		});
}

void AsyncClient::EnqueueLine(const std::string& line)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Lines.push(line);
}

void AsyncClient::Parse(const char* data, size_t len)
{
	m_Incoming.append(data, len);

	size_t pos = 0;
	while ((pos = m_Incoming.find('\n')) != std::string::npos)
	{
		std::string line = m_Incoming.substr(0, pos);
		m_Incoming.erase(0, pos + 1);

		EnqueueLine(line);
	}
}