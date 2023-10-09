#ifndef NET_SERVER_
#define NET_SERVER_

#include "net_base.h"
#include "net_threadsafeQueue.hpp"
#include "net_info.h"
#include "net_connection.h"
#include<iostream>
namespace tl
{
	namespace net
	{
		template<typename T>
		class server_interface
		{
		public:

			server_interface(uint16_t port)
				: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port))

			{
			}

			virtual ~server_interface()
			{
				Stop();
			}

			bool Start()
			{
				try
				{
					//Notice the ordering of events, that's important
					//Because at first we issue some tasks for the ASIO context
					//in order to keep it alive.
					WaitForClientConnection();
					m_threadContext = std::thread([this]() {m_asioContext.run(); });
				}
				catch (std::exception& e)
				{
					// Something prohibited the server from listening
					std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
					return false;
				}

				std::cout << "[SERVER] Started\n";
				return true;

			}

			void Stop()
			{
				//Request context to close. This can take some time.
				m_asioContext.stop();

				//Since it will take some time, we can wait for ASIO and the thread to stop
				//by calling the join()
				if (m_threadContext.joinable()) m_threadContext.join();

				std::cout << "[SERVER] Stopped" << std::endl;


			}

			//This task is for the ASIO context. Asynchronous - Instruct
			//ASIO to wait for connection
			void WaitForClientConnection()
			{
				//Recall: the ASIO context has been associated with the acceptor object
				//With these functions we pass a lambda function which does the work
				//when whatever causes the async_accept() to fire
				m_asioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{
						if (!ec)
						{
							//socket.remote_endpoint() returns the ip address of the newly connected
							//client
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

							//Tell the connection that it is owned by a server
							//and this is simply because we want to tailor how the 
							//connection behaves depending on if it is primarily owned
							//by a server or a client. Both the server and the client
							//will use the same connection object, but there is a slight
							//difference around the edges
							//m_asioContext is the current ASIO Context
							//socket is the socket provided by the async accept function
							//since m_qInfosIn is passed by reference, it becomes shared
							//accross all of the connections.
							//But m_qInfosIn is threadsafe when ading messages to it.
							std::shared_ptr<Connection<T>> newConnection =
								std::make_shared<Connection<T>>(Connection<T>::owner::server,
									m_asioContext, std::move(socket), m_qInfosIn);

							// Give the user server a chance to deny connection
							// By default OnClientConnect() returns false.
							// So the user must provide some sort of override
							// to return true.
							if (OnClientConnect(newConnection))
							{
								//Connection allowed, so add to container of new connections
								m_deqConnections.push_back(std::move(newConnection));

								//Valid connection is assigned their identifier
								m_deqConnections.back()->ConnectToClient(this, nIDCounter++);

								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
							}
							//Here the connection is denied. Also, newConnection is shared_ptr object
							//which when it goes out of scope of this function, will be deleted.
							else
							{
								std::cout << "[-----] Connection Denied\n";
							}
						}
						else
						{
							//Error has occured during acceptance
							std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
						}

						//Calling this function again since we don't want the ASIO
						//context to have nothing to do.
						//It primes ASIO with more work - again simply wait for another
						//connection
						WaitForClientConnection();
					});
			}

			//Send a message to a specific client
			void SendInfoToClient(std::shared_ptr<Connection<T>> client, const info<T>& info)
			{
				//Lets now consider how we send infos to clients.
				//In principle, it's very simple.
				//Firstly, we make sure that the client shared_ptr is valid.
				//Secondly, that the client is still connected. The IsConnected() checks
				//that the socket is still valid.
				if (client && client->IsConnected())
				{
					client->Send(info);
				}
				else
				{
					//We don't neccessarily know when the client has disconnected.
					//Why should we? It's disconnected, it can't send that fact.
					//It is only when we try to manipulate the client and that manipulation fails
					//do we have any idea that the client is no longer there.
					//So by testing to see if the socket is valid earlier, we know if we 
					//can or can't communicate with the client.
					//In the event that we can't communicate with the client, we know that 
					//the client has been disconnected.
					OnClientDisconnect(client);
					//The client is no longer valid so we delete it
					client.reset();
					//Since we can identify deleted clients, we use the std::remove()
					//to entirely remove the client from the deque of connections.
					//If we had many different clients, this erasure could become a very
					//expensive operation. Therefore, we want to take that into account
					//when inofing all connections.
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end()
					);
				}
			}

			//@param pIngoreClient indicates a specifc client to ignore when sending
			//info to all clients

			void SendInfoToAllClients(const info<T>& info, std::shared_ptr<Connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;
				//It is important to notice that we are erasing clients after
				//having iterating through all the clients in m_deqConnections.
				//This is because, we don't want to change the m_deqConnections
				//while iterating since we may invalidate the iterators for the loop
				//and we would be trouble.
				for (auto& client : m_deqConnections)
				{
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
							client->Send(info);
					}
					else
					{
						// The client couldn't be contacted, so assume it has
						//disconnected.
						OnClientDisconnect(client);
						client.reset();
						bInvalidClientExists = true;
					}
				}

				if (bInvalidClientExists)
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end()
					);
			}

			
			void Update(size_t nMaxInfos = -1, bool bWait=false)
			{
				//We don't need the server to occupy 100% of a CPU
				//So we make the server sleep until the input queue
				//has a message
				if (bWait) m_qInfosIn.wait();
				size_t nInfoCount = 0;

				while (nInfoCount < nMaxInfos && !m_qInfosIn.empty())
				{
					std::cout << "Calling OnInfo\n";
					auto info = m_qInfosIn.pop_front();
					std::cout << "info.remote " << info.remote << " info.info " << info.info << std::endl;
					OnInfo(info.remote, info.info);
					nInfoCount++;
				}
			}



		protected:
			// Called when a client connects, you can veto the connection
			// by returning false
			// Here we can put in a check for max number of clients or we can check
			// the client's ip address and ban it.
			virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client)
			{
				return false;
			}

			// Called when a client appears to have disconnected.
			// This can allow us to remove a client when it disconnects.
			virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client)
			{

			}

			// Called when an info arrives
			virtual void OnInfo(std::shared_ptr<tl::net::Connection<T>> client, tl::net::info<T>& info)
			{
				std::cout << "net_server onInfo called\n";
				
			}

		public:

			//Called when a client is validated
			virtual void OnClientValidated(std::shared_ptr<Connection<T>> client)
			{

			}

		protected:
			//Threadsafe Queue for incoming info packets
			threadsafeQueue<owned_info<T>> m_qInfosIn;

			//Conatiner of active validated connections
			std::deque<std::shared_ptr<Connection<T>>> m_deqConnections;

			//The context is shared across all connected clients
			asio::io_context m_asioContext;
			//ASIO Contexts need a thread
			std::thread m_threadContext;

			//Here we get the sockets of the connected clients which needs an ASIO
			//context
			asio::ip::tcp::acceptor m_asioAcceptor;

			//Clients will be identified in the "wider system" via an ID
			//Every client will have a unique identifier.
			//This serves as:
			//1. a unqiue id across the entire system. this id will be sent to the client so that they know their id and
			//potentially they also know about the ids of other clients in the network.
			//2. Eventhough, the clients will have unique ip-addresses and port number which
			//can be used as an identifier, we are not comfortable to sending out that data
			//to the clients. We will also notice that a numeric address is also simpler 
			//to work with rather than an ip-address
			uint32_t nIDCounter = 10000;

		};
	}
}

#endif
