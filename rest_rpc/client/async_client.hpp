#pragma once
#include "detail/async_connection.hpp"
#include "detail/wait_barrier.hpp"
// for rpc in async client
#include "detail/async_rpc_context.hpp"
#include "detail/async_rpc_channel.hpp"
#include "detail/async_rpc_channel_impl.hpp"
// for rpc in async client
#include "detail/async_sub_channel.hpp"
#include "detail/async_client_private.hpp"

namespace timax { namespace rpc
{
	template <typename CodecPolicy>
	class async_client
	{
	public:
		using codec_policy = CodecPolicy;
		using async_client_private_t = async_client_private<codec_policy>;
		using work_ptr = std::unique_ptr<io_service_t::work>;

		template <typename Ret>
		using rpc_task_alias = typed_rpc_task<codec_policy, Ret>;

	public:
		async_client()
			: ios_()
			, ios_work_(std::make_unique<io_service_t::work>(ios_))
			, ios_run_thread_(boost::bind(&io_service_t::run, &ios_))
			, client_private_(ios_)
		{
		}
		
		~async_client()
		{
			ios_work_.reset();
			if (!ios_.stopped())
				ios_.stop();
			if (ios_run_thread_.joinable())
				ios_run_thread_.join();
		}

		template <typename Protocol, typename ... Args>
		auto call(tcp::endpoint endpoint, Protocol const& protocol, Args&& ... args)
		{
			using result_type = typename Protocol::result_type;
			using rpc_task_t = rpc_task_alias<result_type>;
			auto ctx = client_private_.make_rpc_context(endpoint, protocol, std::forward<Args>(args)...);
			return rpc_task_t{ client_private_, ctx };
		}

		template <typename Protocol, typename Func>
		void sub(tcp::endpoint const& endpoint, Protocol const& protocol, Func&& func)
		{
			client_private_.sub(endpoint, protocol, std::forward<Func>(func));
		}

		template <typename Protocol, typename Func, typename EFunc>
		void sub(tcp::endpoint const& endpoint, Protocol const& protocol, Func&& func, EFunc&& efunc)
		{
			client_private_.sub(endpoint, protocol, std::forward<Func>(func), std::forward<EFunc>(efunc));
		}

	private:
		io_service_t				ios_;
		work_ptr					ios_work_;
		std::thread				ios_run_thread_;
		async_client_private_t	client_private_;
	};
} }
