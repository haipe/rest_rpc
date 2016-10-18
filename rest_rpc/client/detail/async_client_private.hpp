#pragma once

namespace timax { namespace rpc 
{
	template <typename CodecPolicy>
	inline auto make_rpc_context(io_service_t& ios, tcp::endpoint const& endpoint,
		std::string const& name, CodecPolicy const&, typename CodecPolicy::buffer_type&& buffer)
	{
		using context_t = rpc_context<CodecPolicy>;
		return std::make_shared<context_t>(ios, endpoint, name, std::move(buffer));
	}

	template <typename CodecPolicy, typename Protocol, typename ... Args>
	inline auto make_rpc_context(io_service_t& ios, tcp::endpoint const& endpoint, 
		CodecPolicy const& cp, Protocol const& protocol, Args&& ... args)
	{
		auto buffer = protocol.pack_args(cp, std::forward<Args>(args)...);
		return make_rpc_context(ios, endpoint, protocol.name(), cp, std::move(buffer));
	}

	template <typename CodecPolicy>
	class async_client_private
	{
	public:
		using codec_policy = CodecPolicy;
		using context_t = rpc_context<codec_policy>;
		using context_ptr = std::shared_ptr<context_t>;
		using rpc_manager_t = rpc_manager<codec_policy>;
		using sub_manager_t = sub_manager<codec_policy>;

	public:
		async_client_private(io_service_t& ios)
			: ios_(ios)
			, rpc_manager_(ios)
			, sub_manager_(ios)
		{
		}

		io_service_t& get_io_service()
		{
			return ios_;
		}

		rpc_manager_t& get_rpc_manager()
		{
			return rpc_manager_;
		}

		sub_manager_t& get_sub_manager()
		{
			return sub_manager_;
		}

		void call(context_ptr& context)
		{
			rpc_manager_.call(context);
		}

		template <typename Protocol, typename ... Args>
		auto make_rpc_context(tcp::endpoint const& endpoint, Protocol const& protocol, Args&& ... args)
		{
			codec_policy cp{};
			return rpc::make_rpc_context(ios_, endpoint, cp, protocol, std::forward<Args>(args)...);
		}

		template <typename Protocol, typename Func>
		void sub(tcp::endpoint const& endpoint, Protocol const& protocol, Func&& func)
		{
			sub_manager_.sub(endpoint, protocol, std::forward<Func>(func));
		}

		template <typename Protocol, typename Func, typename EFunc>
		void sub(tcp::endpoint const& endpoint, Protocol const& protocol, Func&& func, EFunc&& efunc)
		{
			sub_manager_.sub(endpoint, protocol, std::forward<Func>(func), std::forward<EFunc>(efunc));
		}

	private:
		io_service_t&				ios_;
		rpc_manager_t				rpc_manager_;
		sub_manager_t				sub_manager_;
	};

	template <typename CodecPolicy>
	class rpc_task
	{
	public:
		using codec_policy = CodecPolicy;
		using client_private_t = async_client_private<codec_policy>;
		using context_ptr = typename client_private_t::context_ptr;
		
	public:
		rpc_task(client_private_t& client, context_ptr& ctx)
			: client_(client)
			, ctx_(ctx)
			, dismiss_(false)
		{}

		~rpc_task()
		{
			do_call_managed();
		}

		rpc_task(rpc_task&& other)
			: client_(other.client_)
			, ctx_(std::move(other.ctx_))
			, dismiss_(other.dismiss_)
		{
			other.dismiss_ = true;
		}

		void do_call_managed()
		{
			if (!dismiss_)
			{
				client_.call(ctx_);
			}
		}

		void do_call_and_wait()
		{
			if (!dismiss_)
			{
				dismiss_ = true;
				ctx_->create_barrier();
				client_.call(ctx_);
				ctx_->wait();
			}
		}

		client_private_t&		client_;
		context_ptr			ctx_;
		bool					dismiss_;
	};

	template <typename CodecPolicy, typename Ret>
	class typed_rpc_task : protected  rpc_task<CodecPolicy>
	{
	public:
		using codec_policy = CodecPolicy;
		using base_type = rpc_task<CodecPolicy>;
		using result_type = Ret;
		using client_private_t = typename base_type::client_private_t;
		using context_ptr = typename base_type::context_ptr;

	public:
		typed_rpc_task(client_private_t& client, context_ptr& ctx)
			: base_type(client, ctx)
		{
		}

		template <typename F>
		typed_rpc_task&& on_ok(F&& f) &&
		{
			if (nullptr == result_)
			{
				result_ = std::make_shared<result_type>();
			}

			this->ctx_->on_ok = [func = std::forward<F>(f), r = result_](char const* data, size_t size)
			{
				codec_policy codec{};
				*r = codec.template unpack<result_type>(data, size);
				func(*r);
			};

			return std::move(*this);
		}

		template <typename F>
		typed_rpc_task&& on_error(F&& f) &&
		{
			this->ctx_->on_error = std::forward<F>(f);
			return std::move(*this);
		}

		typed_rpc_task&& timeout(duration_t const& t) &&
		{
			this->ctx_->timeout = t;
			return std::move(*this);
		}

		void wait(duration_t const& duration = duration_t::max()) &
		{
			if (!this->dismiss_)
			{
				if (nullptr == result_)
				{
					result_ = std::make_shared<result_type>();
				}

				this->ctx_->on_ok = [r = result_](char const* data, size_t size)
				{
					codec_policy codec{};
					*r = codec.template unpack<result_type>(data, size);
				};
				this->ctx_->on_error = nullptr;
				this->ctx_->timeout = duration;
			}
			this->do_call_and_wait();
		}

		result_type const& get(duration_t const& duration = duration_t::max()) &
		{
			wait(duration);
			return *result_;
		}

	private:
		std::shared_ptr<result_type>	result_;
	};

	template <typename CodecPolicy>
	class typed_rpc_task<CodecPolicy, void> : protected rpc_task<CodecPolicy>
	{
	public:
		using codec_policy = CodecPolicy;
		using base_type = rpc_task<CodecPolicy>;
		using result_type = void;
		using client_private_t = typename base_type::client_private_t;
		using context_ptr = typename base_type::context_ptr;

	public:
		typed_rpc_task(client_private_t& client, context_ptr& ctx)
			: base_type(client, ctx)
		{
		}

		template <typename F>
		typed_rpc_task&& on_ok(F&& f) &&
		{
			this->ctx_->on_ok = [func = std::forward<F>(f)](char const* data, size_t size) { func(); };
			return std::move(*this);
		}

		template <typename F>
		typed_rpc_task&& on_error(F&& f) &&
		{
			this->ctx_->on_error = std::forward<F>(f);
			return std::move(*this);
		}

		typed_rpc_task&& timeout(duration_t const& t) &&
		{
			this->ctx_->timeout = t;
			return std::move(*this);
		}

		void wait(duration_t const& duration = duration_t::max()) &
		{
			if (!this->dismiss_)
			{
				this->ctx_->timeout = duration;
				this->ctx_->on_ok = nullptr;
				this->ctx_->on_error = nullptr;
			}
			this->do_call_and_wait();
		}
	};
} }