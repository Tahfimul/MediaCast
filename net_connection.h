#ifndef NET_CONNECTION_
#define NET_CONNECTION_
#include "net_base.h"
#include "net_threadsafeQueue.hpp"
#include "net_info.h"


namespace tl
{
	namespace net
	{

		//Forward Declare
		//since server interface is not implemented or its implementation is not included from anywhere
		template<typename T>
		class server_interface;

		//std::enable_shared_from_this allows us to provide a shared_ptr to
		//when returning the this pointer.
		template<typename T>
		class Connection : public std::enable_shared_from_this<Connection<T>>
		{
		public:
			enum class owner
			{
				server,
				client
			};

			// Constructor: Specify Owner, connect to context, transfer the socket
			//				Provide reference to incoming message queue

			Connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, threadsafeQueue<owned_info<T>>& qIn)
				:m_asioContext(asioContext), m_socket(std::move(socket)), m_qInfosIn(qIn)
			{
				m_nOwnerType = parent;

				//Construct validation check data
				if (m_nOwnerType == owner::server)
				{
					//Connection is Server -> Client, construct random data for the client
					//to transform and send back for validation
					m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

					//Precalculate the result for checking when the client responds
					m_nHandshakeCheck = scramble(m_nHandshakeOut);
				}

				else if (m_nOwnerType == owner::client)
				{
					// Connection is Client -> Server, so we have nothing to define for the handshake
					m_nHandshakeIn = 0;
					m_nHandshakeOut = 0;
				}
			}

			virtual ~Connection()
			{}

			//Only called by clients
			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				if (m_nOwnerType == owner::client)
					//Request ASIO attempts to connect to an endpoint
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							if (!ec)
							{
								std::cout << "Connected to server\n";
								//ReadHeader();

								//First thing server will do is send packet to be validated 
								//so wait for that and respond
								ReadValidation();
							}
						});
			}
			//Called by clients and server
			void Disconnect()
			{
				if (IsConnected())
					asio::post(m_asioContext, [this]() { m_socket.close(); });
			}
			//Returns if the connection is valid, open, and currently active
			bool IsConnected() const
			{
				return m_socket.is_open();
			}

			void Send(const info<T>& info)
			{
				asio::post(m_asioContext, 
					[this, info](){

						bool bWritingMessage = !m_qInfosOut.empty();

						m_qInfosOut.push_back(info);
						
						if(!bWritingMessage)
							WriteHeader();

					});
			}

			uint32_t GetID() const
			{
				return id;
			}

			void ConnectToClient(tl::net::server_interface<T>* server,  uint32_t uid = 0)
			{
				if (m_nOwnerType == owner::server)
				{
					if (m_socket.is_open())
					{
						id = uid;
						std::cout << "scpket cnnection now open with client from server\n";
						//ReadHeader();
						//A client has attempted to connect to the server, but we wish
						//the client to first validate itself, so first write out the 
						//handshake data to be validated
						WriteValidation();

						//Issue a task to sit and wait asyncshronously for precisely the validation
						//data to be sent back from the client
						ReadValidation(server);
					}
				}
			}
		private:
			//ASYNC - Prime context ready to read an info header 
			void ReadHeader()
			{
				asio::async_read(m_socket, asio::buffer(&m_infoTemporaryIn.header, sizeof(info_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_infoTemporaryIn.header.size > 0)
							{
								std::cout << "m_infoTemporaryIn.header.size > 0\n";
								m_infoTemporaryIn.body.resize(m_infoTemporaryIn.header.size);
								ReadBody();
							}
							else
							{
								AddToIncomingInfoQueue();
							}

						}
						else
						{
							std::cout << "[" << id << "] Read Header Fail.\n";
							//Manually force close scoket
							m_socket.close();
						}
					});
			}

			//ASYNC - Prime context ready to read an info body
			void ReadBody()
			{
				std::cout << "Size of body: " << m_infoTemporaryIn.body.size() << std::endl;
				asio::async_read(m_socket, asio::buffer(m_infoTemporaryIn.body.data(), m_infoTemporaryIn.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							std::cout << "Read body\n";
							AddToIncomingInfoQueue();
						}
						else
						{
							std::cout << "[" << id << "] Read Body Fail.\n";
							//Manually force close scoket
							m_socket.close();
						}
					});
			}

			//ASYNC - Prime context ready to write an info header
			void WriteHeader()
			{
				asio::async_write(m_socket, asio::buffer(&m_qInfosOut.front().header, sizeof(info_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_qInfosOut.front().body.size()>0)
							{
								WriteBody();
							}
							else{
								//We are done with the info in the queue so we remove it.
								m_qInfosOut.pop_front();

								//If there are more messages to send.
								if (!m_qInfosOut.empty())
									WriteHeader();
							}
						}
						else
						{
							std::cout << "[" << id << "] Write Header Fail.\n";
							//Manually force close scoket
							m_socket.close();
						}
					});
			}

			//ASYNC - Prime context ready to write an info body
			void WriteBody()
			{
				asio::async_write(m_socket, asio::buffer(m_qInfosOut.front().body.data(), m_qInfosOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							//We are done with the info in the queue so we remove it.
							m_qInfosOut.pop_front();

							//If there are more messages to send.
							if (!m_qInfosOut.empty())
								WriteHeader();
						}
						else
						{
							std::cout << "[" << id << "] Write Body Fail.\n";
							//Manually force close scoket
							m_socket.close();
						}
					}
					);
			}

			void AddToIncomingInfoQueue()
			{
				std::cout << "added to incoming info queue "<<(int)m_nOwnerType << std::endl;
				if (m_nOwnerType == owner::server)
					m_qInfosIn.push_back({ this->shared_from_this(), m_infoTemporaryIn });
				//In the case m_nOwnerType is a client we are not concerned with tagging the connection with the this->shared_from_this() pointer
				//since the client will only have connection to one endpoint, that's the server, so the tagging is unneccessary.
				//This is an important distinction because we want to enforce that a client can only have one connection. In the client interface
				//the connection will be stored as a single unique pointer, therefore we can't use the shared_from_this() to create a shared pointer for
				//that info object.
				else if (m_nOwnerType == owner::client)
				{
					std::cout << "Added incoming info to queue\n";
					m_qInfosIn.push_back({ nullptr, m_infoTemporaryIn });
				}

				//Since the AddToIncomingInfoQueue() is called when are finished reading an info, so we will use this opportunity to register
				//another task for the ASIO context to perform.
				ReadHeader();

			}

			// "Encrypt" data to be used for handsake
			uint64_t scramble(uint64_t nInput)
			{
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;

				return out ^ 0xC0DEFACE12345678;
			}

			//ASYNC - Used by both the client and server to write validation packet
			void WriteValidation()
			{
				std::cout << "sending validation code\n";
				asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{

							if (m_nOwnerType == owner::client)
								ReadHeader();
						}
						else
						{
							m_socket.close();
						}
					}
					);
			}

			void ReadValidation(tl::net::server_interface<T>* server = nullptr)
			{
				asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
					[this, server](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_nOwnerType == owner::server)
							{
								if (m_nHandshakeIn == m_nHandshakeCheck)
								{
									std::cout << "Client Validated Successfully\n";
									server->OnClientValidated(this->shared_from_this());

									ReadHeader();
								}
								else
								{
									std::cout << "Client Disconnected (Fail Validation)\n";
									m_socket.close();
								}
							}

							else if (m_nOwnerType == owner::client)
							{
								//Solve a puzzle 
								m_nHandshakeOut = scramble(m_nHandshakeIn);
								WriteValidation();
							}
						}
						else
						{
							std::cout << "Client Disconnected (Read Validation)\n";
							m_socket.close();
						}
					}
					);
			}

		protected:
			//Each connection has a unique socket to a remote
			asio::ip::tcp::socket m_socket;

			//Sockets can't function without an IO context.
			//This context is shared with the whole ASIO instance
			asio::io_context& m_asioContext;

			//This queue holds all infos to be sent to the remote side
			//of this connection
			threadsafeQueue<info<T>> m_qInfosOut;

			//This queue holds all infos that have been received from
			// the remote side of this connection. Note: it is a reference
			// as the "owner" of this connection is expected to provide a
			// queue
			threadsafeQueue<owned_info<T>>& m_qInfosIn;
			info<T> m_infoTemporaryIn;
			// The "owner" decides how some of the connection behaves
			owner m_nOwnerType = owner::server;

			uint32_t id = 0;

			//Handshake Validation
			//The value that the connection will send outwards
			uint64_t m_nHandshakeOut = 0;
			//The value that the connection will receive
			uint64_t m_nHandshakeIn = 0;
			//The value used by server to compare whether the value is valid
			uint64_t m_nHandshakeCheck = 0;


		};
	}
}


#endif

