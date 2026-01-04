#include "D3DBoxServer.h"

void Session::Start()
{
	// 세션 키 할당
	Send("ASSIGN " + std::to_string(m_SessionKey) + "\n");

	SendSnapshot();

	DoRead();
}

void Session::Send(const std::string& msg)
{
	auto self = shared_from_this();
	boost::asio::post(
		m_Socket.get_executor(),
		[this, self, msg]()
		{
			bool writing = !m_WriteQueue.empty();
			m_WriteQueue.push_back(msg);
			if (!writing)
				DoWrite();
		}
	);
}

void Session::DoRead()
{
	auto self = shared_from_this();
	m_Socket.async_read_some(
		boost::asio::buffer(m_ReadBuf),
		[this, self](boost::system::error_code ec, std::size_t len)
		{
			if (ec)
			{
				std::cout << "[DISCONNECT] sessionKey = " << m_SessionKey << "\n";
				return;
			}

			m_Incoming.append(m_ReadBuf.data(), len);

			std::size_t pos;
			while ((pos = m_Incoming.find('\n')) != std::string::npos)
			{
				std::string line = m_Incoming.substr(0, pos);
				m_Incoming.erase(0, pos + 1);
				HandleCommand(line);
			}

			DoRead();
		}
	);
}

void Session::DoWrite()
{
	auto self = shared_from_this();
	boost::asio::async_write(
		m_Socket,
		boost::asio::buffer(m_WriteQueue.front()),
		[this, self](boost::system::error_code ec, std::size_t len)
		{
			if (ec)
				return;

			m_WriteQueue.pop_front();
			if (!m_WriteQueue.empty())
				DoWrite();
		}
	);
}

void Session::HandleCommand(const std::string& cmd)
{
	std::istringstream iss(cmd);
	std::string command;
	iss >> command;

	int x, z;
	iss >> x >> z;
	//SPAWN x, z
	if (command == "SPAWN")
	{
		// 세션당 SPAWN 1회 제한
		if (g_Blocks.find(m_SessionKey) != g_Blocks.end())
			return;

		Block b;
		b.sessionKey = m_SessionKey;
		b.x = x;
		b.z = z;

		g_Blocks[m_SessionKey] = b;

		std::cout << "[SPAWN] key = " << m_SessionKey << " (" << x << ", " << z << ")\n";

		Broadcast("SPAWN " +
			std::to_string(b.sessionKey) + " " +
			std::to_string(b.x) + " " +
			std::to_string(b.z) + "\n"
		);
	}
	//MOVE x, z
	else if (command == "MOVE")
	{
		auto it = g_Blocks.find(m_SessionKey);
		if (it == g_Blocks.end())
			return;					// SPAWN 안됨

		Block& b = it->second;

		b.x = x;
		b.z = z;

		std::cout<<"[MOVE] key = " << m_SessionKey << " (" << x << ", " << z << ")\n";

		Broadcast("MOVE " +
			std::to_string(b.sessionKey) + " " +
			std::to_string(b.x) + " " +
			std::to_string(b.z) + "\n"
		);
	}
	else if (command == "LEAVE")
	{
		auto it = g_Blocks.find(m_SessionKey);
		if (it == g_Blocks.end())
			return;

		std::cout << "[LEAVE] key = " << m_SessionKey << "\n";

		g_Blocks.erase(m_SessionKey);

		Broadcast("LEAVE " + std::to_string(m_SessionKey) + "\n");
	}
}

void Session::SendSnapshot()
{
	Send("SNAPSHOT_BEGIN\n");

	for (auto& [key, b] : g_Blocks)
	{
		Send("SPAWN " +
			std::to_string(b.sessionKey) + " " +
			std::to_string(b.x) + " " +
			std::to_string(b.z) + "\n"
		);
	}

	Send("SNAPSHOT_END\n");
}

void Broadcast(const std::string& msg)
{
	for (auto& s : g_Sessions)
		s->Send(msg);
}

void DoAccept(tcp::acceptor& acceptor)
{
	acceptor.async_accept(
		[&](boost::system::error_code ec, tcp::socket socket)
		{
			if (!ec)
			{
				std::cout << "[CONNECT]\n";
				auto session = std::make_shared<Session>(std::move(socket));
				g_Sessions.push_back(session);
				session->Start();
			}
			DoAccept(acceptor);
		}
	);
}
