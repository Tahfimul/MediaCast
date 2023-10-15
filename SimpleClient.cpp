#include "tl_net.h"
#include <iostream>
#include <gtk/gtk.h>
#include <thread>
enum CustomInfoTypes : uint32_t
{
	CONNECTION_ACCEPTED,
	CONNECTION_VERIFIED,
	WEBM,
	GIF,
	AVI,
	MP4,
	MPEG_2,
	M4V,
	FLV
};

enum CustomUserCommands : uint32_t
{
	SEND_SERVER_PING,
	SEND_SERVER_PLAY,
	SEND_SERVER_PAUSE
};


class CustomClient : public tl::net::client_interface<CustomInfoTypes, CustomUserCommands>
{
public:
	CustomClient()
	{
		app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  		
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

	void runWindow()
	{
		g_signal_connect (app, "activate", G_CALLBACK (activate), this);
		g_application_run (G_APPLICATION (app), 0, nullptr);
  		g_object_unref (app);
	}

	~CustomClient()
	{
		std::cout<<"desctructor called for Custom Client\n";
	}

private:
	void pause (gpointer data)
	{
			CustomClient *_this = static_cast<CustomClient*>(data);

			_this->addToUserCommands(CustomUserCommands::SEND_SERVER_PAUSE);
	}

	static void pause_proxy(GtkWidget *widget, gpointer data)
	{
		CustomClient *_this = static_cast<CustomClient*>(data);
		_this->pause(data);
	}

	void play (gpointer data)
	{
			CustomClient *_this = static_cast<CustomClient*>(data);

			_this->addToUserCommands(CustomUserCommands::SEND_SERVER_PLAY);
	}	

	static void play_proxy(GtkWidget *widget, gpointer data)
	{
		CustomClient *_this = static_cast<CustomClient*>(data);
		_this->play(data);
	}

	static void on_open_response (GtkDialog *dialog, int response, gpointer data)
	{
		CustomClient *_this = static_cast<CustomClient*>(data);
	  if (response == GTK_RESPONSE_ACCEPT)
	    {
	      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);

	      g_autoptr (GFile) file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
	      std::string filePath = g_file_get_path(file);
	      std::cout << "file name: " << filePath << std::endl;

	      GtkEntryBuffer* buff = gtk_entry_buffer_new (filePath.c_str(),filePath.size());

		  gtk_text_set_buffer(GTK_TEXT(_this->chosen_file_path_txt), buff);

	    }

	  gtk_window_destroy (GTK_WINDOW (dialog));
	}

	static void choose_file_proxy(GtkWidget *widget, gpointer data)
	{
		CustomClient *_this = static_cast<CustomClient*>(data);
		 GtkWidget *dialog;
		  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

		  dialog = gtk_file_chooser_dialog_new ("Open File",
		                                        _this->parent_window,
		                                        action,
		                                        "_Cancel",
		                                        GTK_RESPONSE_CANCEL,
		                                        "_Open",
		                                        GTK_RESPONSE_ACCEPT,
		                                        NULL);

		  gtk_window_present (GTK_WINDOW (dialog));
//
		  g_signal_connect (dialog, "response",
		                    G_CALLBACK (on_open_response),
		                    data);

	}

	static void activate (GtkApplication* app, gpointer user_data)
		{
			CustomClient *_this = static_cast<CustomClient*>(user_data);

			GtkWidget *window;
			GtkWidget *play_button;
			GtkWidget *pause_button;
			GtkWidget *file_chooser_button;
			GtkWidget *chosen_file_path_txt;

			GtkCssProvider * provider;

			GtkWidget *main_box, *top_box;

			window = gtk_application_window_new (app);
			gtk_window_set_title ((GtkWindow*)window, "Media Cast");
			gtk_window_set_default_size (GTK_WINDOW (window), 800, 800);

			main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

			gtk_window_set_child (GTK_WINDOW (window), main_box);

			top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

			gtk_box_append(GTK_BOX(main_box), top_box);

			play_button = gtk_button_new_with_label("Play");

			g_signal_connect (play_button, "clicked", G_CALLBACK (play_proxy), user_data);

			gtk_box_append(GTK_BOX(top_box), play_button);

			pause_button = gtk_button_new_with_label("Pause");

			g_signal_connect (pause_button, "clicked", G_CALLBACK (pause_proxy), user_data);

			gtk_box_append(GTK_BOX(top_box), pause_button);


			file_chooser_button = gtk_button_new_with_label("Choose file");

			g_signal_connect(file_chooser_button, "clicked", G_CALLBACK(choose_file_proxy), user_data);

			gtk_box_append(GTK_BOX(main_box), file_chooser_button);

			chosen_file_path_txt = gtk_text_new();

			gtk_box_append(GTK_BOX(main_box), chosen_file_path_txt);

			_this->chosen_file_path_txt = chosen_file_path_txt;

			GtkEntryBuffer* buff = gtk_entry_buffer_new ("text",4);

			gtk_text_set_buffer(GTK_TEXT(chosen_file_path_txt), buff);

			gtk_widget_show (window);

			_this->parent_window = GTK_WINDOW (window);
		}

	GtkApplication *app;
	static GtkWindow *parent_window;
	static GtkWidget *chosen_file_path_txt;


};

GtkWindow* CustomClient::parent_window = nullptr;
GtkWidget* CustomClient::chosen_file_path_txt = nullptr;

int main()
{
	std::thread t1;
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
				auto info = c.Incoming().pop_front().info_;

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
					break;
				case CustomInfoTypes::CONNECTION_VERIFIED:
					std::cout<<"creating thread with address: "<<&c<<std::endl;
					t1 = std::thread(&CustomClient::runWindow, &c);
					break;

				}



			}

			if(!c.UserCommands().empty())
			{
				auto user_command = c.UserCommands().pop_front().id;


				switch(user_command)
				{
					case CustomUserCommands::SEND_SERVER_PING:
						c.PingServer();
						break;
					case CustomUserCommands::SEND_SERVER_PLAY:
						std::cout<<"sending play command to server\n";
						break;
					case CustomUserCommands::SEND_SERVER_PAUSE:
						std::cout<<"send pause command to server\n";
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
