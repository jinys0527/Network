#pragma once
#define _WIN32_WINNT 0x0A00
#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>
#include <array>

using boost::asio::ip::tcp;

// World State
struct Block
{
	int sessionKey;
	int x;
	int z;
};

static std::unordered_map<int, Block> g_Blocks;
static int g_NextSessionKey = 1;

class Session;
static std::vector<std::shared_ptr<Session>> g_Sessions;

void Broadcast(const std::string& msg);

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket socket)
		: m_Socket(std::move(socket)),
		  m_SessionKey(g_NextSessionKey++)
	{ 
	}

	void Start();
	void Send(const std::string& msg);

private:
	void SendSnapshot();
	void DoRead();
	void DoWrite();
	void HandleCommand(const std::string& cmd);

private:
	tcp::socket m_Socket;
	int         m_SessionKey;

	std::array<char, 1024>   m_ReadBuf;
	std::string				 m_Incoming;
	std::deque<std::string>  m_WriteQueue;
};

void DoAccept(tcp::acceptor& acceptor);
