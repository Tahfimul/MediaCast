#ifndef NET_DATA_H
#define NET_DATA_H
#include "net_base.h"

namespace tl
{
	namespace net
	{
		//Data Header is sent at the start of all datas. The template allows us to use
		//client provided "enum class" to ensure that the datas are valid at compile time.
		template <typename T>
		struct info_header
		{
			T id{};
			uint32_t size = 0;
		};

		template <typename T>
		class info
		{
			public:
			info_header<T> header{};
			std::vector<uint8_t> body;

			/*size_t size() const
			{
				return sizeof(info_header<T>) + body.size();
			}*/

			size_t size() const
			{
				return body.size();
			}

			//Overide for std::cout compatability - produces friendly description of data
			friend std::ostream& operator << (std::ostream& os, const info<T>& info)
			{
				os << "ID:" << int(info.header.id) << " Size: " << info.header.size<<std::endl;
				return os;
			}

			//Pushes any POD(Plain Old Data)-like data into data buffer
			//So we assume that the datatype of Infotype should be POD
			//Since POD is both standard layout and trivial, we can check to make sure that 
			//Infotype data struture is standard layout.
			//The reason we are interested in Infotype being standard layout is becuase
			//we can then trivially serialize (here trivailly serializing means that
			//all data members of Infotype data structure would be serialized) the data.
			template<typename Infotype>
			friend info<T>& operator<<(info<T>& info, const Infotype& infodata)
			{

				// Check that the type of the data being pushed is trivially copyable
				static_assert(std::is_standard_layout<Infotype>::value, "Data is not simple enough to be pushed into body vector");

				//Pointer to end of current body vector in data object, this will be used to push data into body vector later
				size_t s = info.body.size();

				//Change size of body vector in accordance to size of new data, infodata, that will be pushed to body vector
				info.body.resize(s + sizeof(Infotype));

				//Copy data from infodata object into body vector of data object
				std::memcpy(info.body.data() + s, &infodata, sizeof(Infotype));

				//Change size of data, so header accurately reflects size of data
				info.header.size = info.size();

				// Return the target data so it can be "chained"
				// "Chaining" is when a method returns a reference to an object
				// so that another method to that object can be called
				// Ex. object<<a<<b<<c
				return info;

			}

			//Pops any POD(Plain Old Data)-like data from data buffer
			//So we assume that the datatype of Infotype should be POD
			//Since POD is both standard layout and trivial, we can check to make sure that 
			//Infotype data struture is standard layout.
			//The reason we are interested in Infotype being standard layout is becuase
			//we can then trivially serialize (here trivailly serializing means that
			//all data members of Infotype data structure would be serialized) the data.

			template<typename Infotype>
			friend info<T>& operator>>(info<T>& info, Infotype& infodata)
			{
				// Check that the type of the data being pushed is trivially copyable
				static_assert(std::is_standard_layout<Infotype>::value, "Data is not simple enough to be popped out of body vector");

				//Pointer to beginning of data to be popped
				size_t s = info.body.size() - sizeof(Infotype);

				//Copy data from body vector of data object into infodata object
				std::memcpy(&infodata, info.body.data() + s, sizeof(Infotype));

				//Change size of body vector in accordance to size of new data that was read from body vector
				//into infodata, that will be popped out of body vector
				info.body.resize(s);

				//Change size of data, so header accurately reflects size of data
				info.header.size = info.size();
				
				// Return the target data so it can be "chained"
				// "Chaining" is when a method returns a reference to an object
				// so that another method to that object can be called
				// Ex. object<<a<<b<<c
				return info;

			}


		};

		//Forward declare the connection
		template<typename T>
		class Connection;

		template <typename T>
		class owned_info
		{
		public:
			//The server may need to respond back to the client and for that it needs to know where the client contacted the server 
			//from, so essentially this shared_ptr serves as a tag to the client.
			//This shared_ptr can also be used as a tag for the client to tag and communicate with the server. However, that is 
			//redundant since the client already will know the endpoint to connect to in order to communicate with the server.
			std::shared_ptr<Connection<T>> remote = nullptr;
			
			info<T> info;

			friend std::ostream& operator<<(std::ostream& os, const owned_info<T>& info)
			{
				os << info.info;
				return os;
			}
			
		};



	}
}



#endif