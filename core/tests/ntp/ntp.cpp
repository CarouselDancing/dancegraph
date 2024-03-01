#include <ntp_client/ntp.h>
#include <spdlog/spdlog.h>
using namespace CTU::VIN::NTP_client;

int main(int argc, const char** argv)
{
	::Client client;
	char buf[1024];
	while (true)
	{
		enum Status s;
		struct Result* result = nullptr;
		HNTP client = Client__create();
		const char* hostname = "130.88.202.49";
		//const char* hostname = "195.113.144.201";
		s = Client__query(client, hostname, &result);
		Client__format_info_str(result, buf);
		spdlog::info("RESULTS: {}\n", buf);
		Client__free_result(result);
		Client__close(client);
	}
}