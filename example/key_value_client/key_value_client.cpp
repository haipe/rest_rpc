#include <rest_rpc/client.hpp>
#include <fstream>

namespace kv
{
	using namespace std::string_literals;

	TIMAX_DEFINE_PROTOCOL(get, std::vector<char>(std::string const&));
	TIMAX_DEFINE_PROTOCOL(put, bool(std::string const&, std::vector<char> const&));

	enum class operation
	{
		put,
		get,
		unknown,
	};

	operation get_operation(char const* operation)
	{
		if ("put"s == operation)
			return operation::put;
		else if("get"s == operation)
			return operation::get;
		else
			return operation::unknown;
	}

	using client_t = timax::rpc::sync_client<timax::rpc::msgpack_codec>;

	int put_operation(int argc, char* argv[])
	{
		if (5 != argc)
		{
			std::cout << "Args not match!" << std::endl;
			return -1;
		}

		auto const operation = argv[2];
		std::string key(argv[3]);
		std::vector<char> value;
		if ("string"s == operation)
		{
			auto len = std::strlen(argv[4]);
			if (0 == len)
			{
				std::cout << "Cannot send a null value." << std::endl;
				return -1;
			}
			value.resize(len + 1);
			std::copy(argv[4], argv[4] + value.size(), value.data());
		}
		else if ("file"s == operation)
		{
			std::fstream file;
			file.open(argv[4], std::ios::in | std::ios::binary);
			if (!file)
			{
				std::cout << "File not exists!" << std::endl;
				return -1;
			}
			file.seekg(std::ios::end);
			auto size = file.tellg();
			if (size > 102400)
			{
				std::cout << "Too big for rpc to send." << std::endl;
				return -1;
			}
			if (size <= 0)
			{
				std::cout << "Cannot send a null value." << std::endl;
				return -1;
			}
			file.seekg(std::ios::beg);
			value.resize(size);
			file.read(value.data(), value.size());
		}
		else
		{
			std::cout << "Unknown operation!" << std::endl;
			return -1;
		}
			

		client_t client;
		auto endpoint = timax::rpc::get_tcp_endpoint("127.0.0.1", 9000);
		
		try
		{
			auto result = client.call(endpoint, put, key, value);
			if (!result)
			{
				std::cout << "Failed to store object." << std::endl;
				return -1;
			}
				
		}
		catch (timax::rpc::exception const& exception)
		{
			std::cout << "Failed to store object, exception:" << exception.get_error_message() << std::endl;
			return -1;
		}
		
		return 0;
	}

	int get_operation(int argc, char* argv[])
	{
		if (4 > argc)
			return -1;

		auto const operation = argv[2];
		std::string key = argv[3];

		client_t client;
		auto endpoint = timax::rpc::get_tcp_endpoint("127.0.0.1", 9000);

		if ("string"s == operation)
		{
			auto buffer = client.call(endpoint, get, key);
			std::cout << buffer.data() << std::endl;
		}
		else if ("file"s == operation)
		{
			if (argc != 5)
			{
				std::cout << "Args not match." << std::endl;
				return -1;
			}

			auto buffer = client.call(endpoint, get, key);
			auto filepath = argv[4];

			std::fstream file;
			file.open(filepath, std::ios::out | std::ios::binary);
			file.write(buffer.data(), buffer.size());
			file.close();
		}
		else
		{
			std::cout << "Unknown operation!" << std::endl;
			return -1;
		}

		return 0;
	}
}

int main(int argc, char* argv[])
{
	if (2 > argc)
		return -1;

	auto operation = kv::get_operation(argv[1]);
	if (kv::operation::unknown == operation)
		return -1;

	switch (operation)
	{
	case kv::operation::put:
		return kv::put_operation(argc, argv);
	case kv::operation::get:
		return kv::get_operation(argc, argv);
	default:
		std::cout << "Unknown operation!" << std::endl;
		return -1;
	}


	return 0;
}