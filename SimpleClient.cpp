#include "tl_net.h"
#include <iostream>
#include <gtk/gtk.h>

enum CustomInfoTypes : uint32_t
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


class CustomClient : public tl::net::client_interface<CustomInfoTypes>
{
public:
	bool getVideo(const char* title)
	{
		/*tl::net::info<Video> info;
		info.header.id = Video::GIF;
		info << title;
		Send(info);*/
		return true;
	}

	void PingServer()
	{
		tl::net::info<CustomInfoTypes> info;
		info.header.id = CustomInfoTypes::GIF;

		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		
		info << timeNow;
		std::cout << "pining server\n";
		Send(info);
	}

};

int main()
{

	CustomClient c;
	c.Connect("127.0.0.1", 60000);

	bool bQuit = false;

	while (!bQuit)
	{

		if (c.IsConnected())
		{
		
			if (!c.Incoming().empty())
			{
				std::cout << "received incoming packet\n";
				auto info = c.Incoming().pop_front().info;

				std::cout << "incoming packet header id: " << info.header.id << std::endl;

				switch (info.header.id)
				{
				case CustomInfoTypes::GIF:
					{
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					info >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << std::endl;
				
					}				
					break;
				case CustomInfoTypes::CONNECTION_ACCEPTED:
					std::cout << "Server Accepted Connection\n";
					//c.PingServer();
					break;

				}
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}
	}

	return 0;


}