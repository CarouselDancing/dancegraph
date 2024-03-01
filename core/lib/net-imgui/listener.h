#pragma once

#include <core/net/listener.h>
namespace net
{
	class ListenerGui : public ListenerState
	{
	public:
		ListenerGui(const char* application_name);
		void on_gui() override;
	private:
		void init_gui();
		void run_gui();

		const char* application_name = nullptr;
	};
}
