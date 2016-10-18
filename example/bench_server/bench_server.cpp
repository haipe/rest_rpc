#include <rest_rpc/server.hpp>

namespace bench
{
	int add(int a, int b)
	{
		return a + b;
	}

	void some_task_takes_a_lot_of_time(double, int)
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(5s);
	}

	enum class type_t
	{
		connection,
		operation,
	};

	template <typename Server>
	void count_qps(Server& server, std::atomic<uint64_t>& qps)
	{
		server.register_handler("add", bench::add,
			[&qps](auto, auto)
		{
			++qps;
		});

		server.register_handler("sub_add", bench::add, [&server, &qps](auto, auto r)
		{
			++qps;
			server.pub("sub_add", r, [&qps]
			{
				++qps;
			});
		});

		std::thread{ [&qps]
		{
			while (true)
			{
				using namespace std::chrono_literals;

				std::cout << "QPS: " << qps.load() << ".\n";
				qps.store(0);
				std::this_thread::sleep_for(1s);
			}

		} }.detach();
	}

	template <typename Server>
	void count_connection(Server& server, std::atomic<uint64_t>& conn_count)
	{
		server.register_handler("bench_conn", [&conn_count]()
		{
			++conn_count;
		});

		std::thread{ [&conn_count]
		{
			while (true)
			{
				using namespace std::chrono_literals;

				std::cout << "Connection: " << conn_count.load() << ".\n";
				std::this_thread::sleep_for(1s);
			}

		} }.detach();
	}
}

int main(int argc, char *argv[])
{
	using namespace std::chrono_literals;
	using server_t = timax::rpc::server<timax::rpc::msgpack_codec>;

	if (3 != argc)
	{
		std::cout << "Usage: " << "$ ./bench_server %d(0 or 1) %d(your port number)" << std::endl;
		return -1;
	}

	timax::log::get().init("bench_server.lg");

	std::atomic<uint64_t> work_count{ 0 };
	auto bench_type = static_cast<bench::type_t>(boost::lexical_cast<int>(argv[1]));
	auto port = boost::lexical_cast<uint16_t>(argv[2]);
	auto pool_size = static_cast<size_t>(std::thread::hardware_concurrency());

	server_t server{ port, pool_size };

	switch (bench_type)
	{
	case bench::type_t::connection:
		bench::count_connection(server, work_count);
		break;
	case  bench::type_t::operation:
		bench::count_qps(server, work_count);
		break;
	}
	
	server.start();
	std::getchar();
	server.stop();
	return 0;
}
