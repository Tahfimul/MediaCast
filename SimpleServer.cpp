#include <iostream>
#include <tl_net.h>

enum class CustomInfoTypes : uint32_t
{
	CONNECTION_ACCEPTED,
	WEBM,
	GIF,
	AVI,
	MP4,
	MPEG_2,
	M4V,
	FLV
};


class CustomServer : public tl::net::server_interface<CustomInfoTypes>
{
public:
	CustomServer(uint16_t nPort) : tl::net::server_interface<CustomInfoTypes>(nPort)
	{

	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<tl::net::Connection<CustomInfoTypes>> client)
	{
		tl::net::info<CustomInfoTypes> info;
		info.header.id = CustomInfoTypes::CONNECTION_ACCEPTED;
		client->Send(info);	
		return true;
	}

	virtual void OnClientDisconnect(std::shared_ptr<tl::net::Connection<CustomInfoTypes>> client)
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}
	virtual void OnInfo(std::shared_ptr<tl::net::Connection<CustomInfoTypes>> client, tl::net::info<CustomInfoTypes>& info) 
	{
		std::cout << "On Info\n";
		switch (info.header.id)
		{
		case CustomInfoTypes::GIF:
			std::cout << "[" << client->GetID() << "]: Server GIF Ping\n";

			client->Send(info);
			break;
		}
	}




};

int main()
{
	CustomServer server(60000);
	server.Start();

	while (1)
	{
		server.Update(-1, true);
	}
	return 0;
}