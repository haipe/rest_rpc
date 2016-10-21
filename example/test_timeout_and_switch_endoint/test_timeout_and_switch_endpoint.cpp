#include <rest_rpc/client.hpp>

namespace client
{
	TIMAX_DEFINE_PROTOCOL(add, int(int, int));
}

int main(void)
{
	using namespace std::chrono_literals;
	using client_t = timax::rpc::async_client<timax::rpc::msgpack_codec>;

	auto endpoints = timax::rpc::get_tcp_endpoints("192.168.2.204:9000|192.168.2.237:9000");
	auto itr = endpoints.begin();

	client_t async_client;

	while (true)
	{
		try
		{
			auto task = async_client.call(*itr, client::add, 1, 2);
			auto r = task.get(20s);
			assert(3 == r);
		}
		catch (timax::rpc::exception const& error)
		{
			auto ec = error.get_error_code();
			if (ec == timax::rpc::error_code::BADCONNECTION ||
				ec == timax::rpc::error_code::TIMEOUT)
			{
				++itr;
				if (itr == endpoints.end())
					itr = endpoints.begin();
			}
			else
			{
				break;
			}
		}

		std::this_thread::sleep_for(1s);
	}

	return 0;
}