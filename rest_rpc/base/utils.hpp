#pragma once

namespace timax{ namespace rpc 
{
	inline bool retry(const std::function<bool()>& func, size_t max_attempts, size_t retry_interval = 0)
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

	template<typename T>
	inline std::string get_json(result_code code, const T& r, std::string const& tag)
	{
		Serializer sr;
		response_msg<T> msg = { static_cast<int>(code), r };
		sr.Serialize(msg);

		std::string result = sr.GetString();

		if (!tag.empty())
		{
			auto pos = result.rfind('}');
			assert(pos != std::string::npos);
			result.insert(pos, tag);
		}

		return std::move(result);
	}

	inline std::vector<tcp::endpoint> get_tcp_endpoints(std::string const& address_port_string_list)
	{
		std::vector<std::string> address_port_list;
		boost::split(address_port_list, address_port_string_list, boost::is_any_of(" ,|"));
		std::vector<tcp::endpoint> tcp_endpoints;
		std::transform(address_port_list.begin(), address_port_list.end(), std::back_inserter(tcp_endpoints),
			[](auto const& address_port) -> tcp::endpoint
		{
			auto pos = address_port.rfind(':');
			if (pos == std::string::npos)
				throw std::runtime_error{ "Bad address format!" };
			return
			{
				boost::asio::ip::address::from_string(address_port.substr(0, pos)),
				boost::lexical_cast<uint16_t>(address_port.substr(pos + 1))
			};
		});

		return tcp_endpoints;
	}

	static tcp::endpoint get_tcp_endpoint(std::string const& address, uint16_t port)
	{
		return{ boost::asio::ip::address::from_string(address), port };
	}
} }