#pragma once

#include <core/net/server.h>
#include <sig/signal_common.h>


namespace net
{
	class ServerGui : public ServerState
	{
	public:
		ServerGui(const char * application_name);
		void on_gui() override;

	private:
		void init_gui();
		void run_gui();

		const char* application_name = nullptr;
	};
}
