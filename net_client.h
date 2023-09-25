#ifndef NET_CLIENT_
#define NET_CLIENT_

#include "net_base.h"
#include "net_info.h"
#include "net_threadsafeQueue.hpp"
#include "net_connection.h"

namespace tl
{
	namespace net
	{
		//This class is responsible for setting up ASIO and Connection.
		//It also acts as an accesspoint for the application to talk to 
		//the server.
		template <typename T>
		class client_interface
		{
		
		public:
			//Initialize the socket with the io context, so it can do stuff
			client_interface() : m_socket(m_context)
			{

			}

			virtual ~client_interface()
			{
				//If the client is destroyed, always try and disconnect from
				//server
				Disconnect();
			}
			// Connect to server with hostname/ip-address and port
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					//Create connection
					m_connection = std::make_unique<connection<T>>();

					//Resolve hostname/ip-address into tangible physical
					//address
					asio::ip::tcp::resolver resolver(m_context);
					m_endpoints = resolver.resolve(host, std::to_string(port));

					//Tell the connection object to connect to server
					m_connection->ConnectToServer(m_endpoints);

					// Start context thread
					thrContext = std::thread([this]() {m_context.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "Client Exception: " << e.what() << std::endl;
					return false;
				}
				return true;
			}

			//Disconnect from the server
			void Disconnect()
			{
				//If connection exists, and it's connected, then disconnect
				//from server gracefully
				if (IsConnected())
					m_connection->Disconnect();

				//We are also done with the ASIO context and its thread
				m_context.stop();

				if (thrContext.joinable())
					thrContext.join();

				//Destroy the connection object by releasing the unique pointer
				m_connection.release();
			}

			// Check if client has a valid, open, and currently active connection
			// to a server
			bool IsConnected()
			{
				if (m_connection)
					return m_connection->IsConnected();
				else
					return false;
			}
			
			// Retrieve queue of messages from server
			threadsafeQueue<owned_info<T>>& Incoming()
			{
				return m_qMessagesIn;
			}

			//Send info to server
			void Send(const info<T>& info)
			{
				if (IsConnected())
					m_connection->Send(info);
			}
		protected:
			//ASIO context ahndles the data transfer
			asio::io_context m_context;
			//The context alone doesn't do very much. It needs a thread of its own
			//to execute its work commands.
			std::thread thrContext;

			//This is the hardware socket that is connected to the server
			asio::ip::tcp::socket m_socket;

			//If a connection can be established, the client will have a
			//single instance of a "connection" object, which handles
			//data transfer. Once the connection object is created, the client
			//interface will hand over the ASIO stuff to the connection.
			std::unique_ptr<Connection<T>> m_connection;

		private:
			//This is the thread safe queue of the incoming messages from server.
			threadsafeQueue<owned_info<T>> m_qMessagesIn;
		};
	}
}

#endif