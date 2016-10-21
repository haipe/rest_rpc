#include <rest_rpc/server.hpp>

class kv_store
{
public:
	using server_t = timax::rpc::server<timax::rpc::msgpack_codec>;
	using kv_container_t = std::map<std::string, std::vector<char>>;
	using lock_t = std::unique_lock<std::mutex>;

public:
	explicit kv_store()
		: server_(9000, std::thread::hardware_concurrency())
	{

	}

	void start()
	{
		server_.register_handler("put", timax::bind(&kv_store::put, this));
		server_.register_handler("get", timax::bind(&kv_store::get, this));

		server_.start();
	}

	void stop()
	{
		server_.stop();
	}

	bool put(std::string&& key, std::vector<char>&& value)
	{
		lock_t lock{ mutex_ };
		auto itr = storage_.find(key);
		if (itr == storage_.end())
		{
			storage_.emplace(std::move(key), std::move(value));
			return true;
		}
		itr->second = std::move(value);
		return false;
	}

	std::vector<char> get(std::string const& key)
	{
		lock_t lock{ mutex_ };
		auto itr = storage_.find(key);
		if (itr == storage_.end())
			return{};
		std::vector<char> to_return = std::move(itr->second);
		storage_.erase(itr);
		return to_return;
	}

	void remove(std::string const& key)
	{
		lock_t lock{ mutex_ };
		auto itr = storage_.find(key);
		if (itr != storage_.end())
			storage_.erase(itr);
	}

private:
	server_t					server_;
	kv_container_t			storage_;
	std::mutex				mutex_;
};

int main(void)
{
	kv_store kv_store_service;

	kv_store_service.start();
	std::getchar();
	kv_store_service.stop();
}