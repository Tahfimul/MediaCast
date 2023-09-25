#include "tl_net.h"
#include <iostream>

enum Video : uint32_t
{
	WEBM,
	GIF,
	AVI,
	MP4,
	MPEG_2,
	M4V,
	FLV
};

class CustomClient : public tl::net::client_interface<Video>
{
public:
	bool getVideo(const char* title)
	{
		tl::net::info<Video> info;
		info.header.id = Video::GIF;
		info << title;
		Send(info);
	}

};

int main()
{

	CustomClient c;
	c.Connect("somewhere.com", 80);
	c.getVideo("The Message");



}