#include <rest_rpc/client.hpp>

namespace client
{
	TIMAX_DEFINE_PROTOCOL(add, int(int, int));
}

int main(void)
{
	auto endpoints = timax::rpc::get_tcp_endpoints("192.168.2.204:9000|192.168.2.");

	return 0;
}