#pragma once

namespace timax { namespace rpc 
{
	template <typename CodecPolicy>
	class router : boost::noncopyable
	{
	public:
		using codec_policy = CodecPolicy;
		using message_t = typename codec_policy::buffer_type;
		using connection_ptr = std::shared_ptr<connection>;
		using invoker_t = std::function<void(connection_ptr, char const*, size_t)>;
		using invoker_container = std::unordered_map<uint64_t, invoker_t>;

	public:
		template <typename Handler, typename PostFunc>
		bool register_invoker(std::string const& name, Handler&& handler, PostFunc&& post_func)
		{
			using handler_traits_type = handler_traits<Handler>;
			using invoker_traits_type = invoker_traits<
				typename handler_traits_type::return_tag, handler_exec_sync>;

			return register_invoker_impl<invoker_traits_type>(name,
				std::forward<Handler>(handler), std::forward<PostFunc>(post_func));
		}

		template <typename Handler>
		bool register_invoker(std::string const& name, Handler&& handler)
		{
			using handler_traits_type = handler_traits<Handler>;
			using invoker_traits_type = invoker_traits<
				typename handler_traits_type::return_tag, handler_exec_sync>;

			return register_invoker_impl<invoker_traits_type>(name, std::forward<Handler>(handler));
		}

		template <typename Handler, typename PostFunc>
		bool async_register_invoker(std::string const& name, Handler&& handler, PostFunc&& post_func)
		{
			using handler_traits_type = handler_traits<Handler>;
			using invoker_traits_type = invoker_traits<
				typename handler_traits_type::return_tag, handler_exec_async>;

			return register_invoker_impl<invoker_traits_type>(name,
				std::forward<Handler>(handler), std::forward<PostFunc>(post_func));
		}

		template <typename Handler>
		bool async_register_invoker(std::string const& name, Handler&& handler)
		{
			using handler_traits_type = handler_traits<Handler>;
			using invoker_traits_type = invoker_traits<
				typename handler_traits_type::return_tag, handler_exec_async>;

			return register_invoker_impl<invoker_traits_type>(name, std::forward<Handler>(handler));
		}

		template <typename Handler>
		bool register_forward_invoker(std::string const& name, Handler&& handler)
		{
			invoker_t invoker = [h = std::forward<Handler>(handler)]
				(connection_ptr conn, char const* data, size_t size)
			{
				h(data, size);
				auto ctx = context_t::make_message(conn->head_, context_t::message_t{});
				conn->response(ctx);
			};
			
			return register_invoker_impl(name, std::move(invoker));
		}

		template <typename Handler>
		bool register_raw_invoker(std::string const& name, Handler&& handler)
		{
			return register_invoker_impl(name, std::forward<Handler>(handler));
		}

		bool has_invoker(std::string const& name) const
		{
			return invokers_.find(name) != invokers_.end();
		}

		void apply_invoker(connection_ptr conn, char const* data, size_t size) const
		{
			static auto cannot_find_invoker_error = codec_policy{}.pack(exception{ error_code::FAIL, "Cannot find handler!" });

			auto& header = conn->get_read_header();
			auto itr = invokers_.find(header.hash);
			if (invokers_.end() == itr)
			{
				auto ctx = context_t::make_error_message(header, cannot_find_invoker_error);
				conn->response(ctx);
			}
			else
			{
				auto& invoker = itr->second;
				if (!invoker)
				{
					auto ctx = context_t::make_error_message(header, cannot_find_invoker_error);
					conn->response(ctx);
				}

				try
				{
					invoker(conn, data, size);
				}
				catch (exception const& error)
				{
					// response serialized exception to client
					auto args_not_match_error = codec_policy{}.pack(error);
					auto args_not_match_error_message = context_t::make_error_message(header,
						std::move(args_not_match_error));
					conn->response(args_not_match_error_message);
				}
			}
		}

	private:
		template <typename InvokerTraits, typename ... Handlers>
		bool register_invoker_impl(std::string const& name, Handlers&& ... handlers)
		{
			auto hash = hash_(name);
			auto itr = invokers_.find(hash);
			if (invokers_.end() != itr)
				return false;

			auto invoker = InvokerTraits::template get<codec_policy>(std::forward<Handlers>(handlers)...);
			invokers_.emplace(hash, std::move(invoker));
			return true;
		}

		template <typename Handler>
		bool register_invoker_impl(std::string const& name, Handler&& handler)
		{
			auto hash = hash_(name);
			auto itr = invokers_.find(hash);
			if (invokers_.end() != itr)
				return false;

			invoker_t invoker = std::forward<Handler>(handler);
			invokers_.emplace(hash, std::move(invoker));
			return true;
		}

	private:
		// mutable std::mutex		mutex_;
		invoker_container			invokers_;
		std::hash<std::string>	hash_;
	};
} }