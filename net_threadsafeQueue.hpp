#ifndef NET_THREADSAFEQUEUE_HPP
#define NET_THREADSAFEQUEUE_HPP
/*
	net_threadsafeQueue.hpp

	The reason why we need a threadsafe Queue is because of the way our client/server architecture is structured.

	|----------|   |-------------------|   |-------------------|   |----------|
	||------|  | - |			  -----| - |-----         -----| - |  |------||
	| Queue   /|In |/-------------|    |Out|    |/-------|Queue|Out|/ Running |
	||------| \| - |\-------------|    | - |    |\-------|-----| - |\ Program |
	|   \/     |   |              |   A|S I|O   |			   |   |  |------||
	||------|  | - |  -----       |    | - |    |			   | - ||------|  |
	|Running  \|Out|\|Queue|-----\|----|Out|----|-------------\|In | Queue	  |
	|Program  /| - |/|-----|-----/|----| - |----|-------------/| - ||------|  |
	||------|  |   |	          |    |   |    |			   |   |		  |
	|Client	   |   | Connection   |----|   |----| Connection   |   |Server	  |
	 ----------     -------------------     -------------------     ----------

	 At any given time, the Queue in the Client can be accessed by the Connection or the Running Program in the Client. Also in the
	 case of the server, there will be musltiple Connections from multiple Clients and at any given time, each Connection may 
	 access the Queue in the Server or the Queue may be accessed any time by the Running program in the Server. And these accesses 
	 are happening at various points in time and it is likely that we don't have control over when these accesses will occur since
	 data takes time to move from Server to Client and viceversa.Also given that we are basing these communications using an
	 Asynchronous Input Output (ASIO) library, as we take these parallel, sporadic requests and trying to serialize/deserialize them 
	 into/from a queue, we need no ensure that the queue is threadsafe.
*/

#include "net_base.h"

namespace tl
{
	namespace net
	{
		//We will design a threadsafe queue with locks which means as something is trying to write to the queue, nothing else
		//can read from it. 
		//Since we are designing a threadsafe queue, we also design it such that it can accept any type of data not just infos.

		template<typename T>
		class threadsafeQueue
		{
			public:
				threadsafeQueue() = default;

				//We won't allow the queue to be copied using a deleted function, primarily because it has mutex object in it.
				//Secondly, because implementing a threadafe queue is just a case of adding guarding to the standard functions 
				//provided to the double ended queue.
				threadsafeQueue(const threadsafeQueue<T>&) = delete;

				virtual ~threadsafeQueue()
				{
					clear();
				}

				//Returns and maintains item at front of Queue
				
				const T& front()
				{
					//To prevent anything else from running while returned object reference is being returned.
					std::scoped_lock lock(muxQueue);
					return deqQueue.front();
				}

				//Returns and maintains item at back of Queue
				const T& back()
				{
					//To prevent anything else from running while returned object reference is being returned.
					std::scoped_lock lock(muxQueue);
					return deqQueue.back();
				}

				// Adds an item to back of queue
				void push_back(const T& item)
				{
					//To prevent anything else from running while adding item to back of queue
					std::scoped_lock lock(muxQueue);
					deqQueue.emplace_back(std::move(item));
				}

				//Adds item to front of Queue

				void push_front(const T& item)
				{
					//To prevent anything else from running while adding item to front of queue
					std::scoped_lock lock(muxQueue);
					deqQueue.emplace_front(std::move(item));
				}

				//Returns if Queue has no items
				bool empty()
				{
					//To prevent anything else from running while returning whether Queue is empty
					std::scoped_lock lock(muxQueue);
					return deqQueue.empty();
				}

				// Returns number of items in Queue
				size_t size()
				{
					//To prevent anything else from running while returning size of Queue
					std::scoped_lock lock(muxQueue);
					return deqQueue.size();
				}

				// Clears Queue
				void clear()
				{
					//To prevent anything else from running while Queue is being cleared
					std::scoped_lock lock(muxQueue);

					deqQueue.clear();
				}

				//Removes and returns item from front of Queue
				T pop_front()
				{
					//To prevent anything else from running while Queue removes and returns item from front of Queue
					std::scoped_lock lock(muxQueue);
					auto t = std::move(deqQueue.front());
					deqQueue.pop_front();
					return t;

				}

				//Reomves and retuns item from back of Queue
				T pop_back()
				{
					//To prevent anything from running while Queue removes and retuns item from back of Queue
					std::scoped_lock lock(muxQueue);
					auto t = std::move(deqQueue.back());
					deqQueue.pop_back();
					return t; 
				}





			protected:
				//This is a standard mutex. This will be used to protect access to the double ended queue.
				std::mutex muxQueue;
				//This is a standard double ended queue
				std::deque<T> deqQueue;

		};
	}
}

#endif