#ifndef NET_CONNECTION_
#define NET_CONNECTION_
#include "net_base.h"
#include "net_threadsafeQueue.hpp"
#include "net_info.h"


namespace tl
{
	namespace net
	{
		//std::enable_shared_from_this allows us to provide a shared_ptr to
		//when returning the this pointer.
		template<typename T>
		class Connection : public std::enable_shared_from_this<Connection<T>>
		{
		public:
			Connection()
			{}

			virtual ~Connection()
			{}

			//Only called by clients
			bool ConnectToServer();
			//Called by clients and server
			bool Disconnect();
			//Returns if the connection is valid, open, and currently active
			bool IsConnected() const;

			bool Send(const info<T>& info);
		protected:
			//Each connection has a unique socket to a remote
			asio::ip::tcp::socket m_socket;

			//Sockets can't function without an IO context.
			//This context is shared with the whole ASIO instance
			asio::io_context& m_asioContext;

			//This queue holds all infos to be sent to the remote side
			//of this connection
			threadsafeQueue<info<T>> m_qMessagesOut;

			//This queue holds all infos that have been received from
			// the remote side of this connection. Note: it is a reference
			// as the "owner" of this connection is expected to provide a
			// queue
			threadsafeQueue<owned_info<T>>& m_qMessagesIn;
		};
	}
}


#endif

