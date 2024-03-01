#pragma once

#include <core/net/client.h>
namespace net
{
	class ClientGui : public ClientState
	{
	public:
		ClientGui(const char* application_name);
		void on_gui() override;
	private:
		void init_gui();
		void run_gui();

		const char* application_name = nullptr;
	};
}
