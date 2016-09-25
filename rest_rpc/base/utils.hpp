#pragma once

namespace timax{ namespace rpc 
{
	static bool retry(const std::function<bool()>& func, size_t max_attempts, size_t retry_interval = 0)
	{
		for (size_t i = 0; i < max_attempts; i++)
		{
			if (func())
				return true;

			if (retry_interval > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval));
		}

		return false;
	}

	static tcp::endpoint get_tcp_endpoint(std::string const& address, uint16_t port)
	{
		return{ boost::asio::ip::address::from_string(address), port };
	}
} }