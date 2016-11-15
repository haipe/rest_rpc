#pragma once

namespace msgpack { MSGPACK_API_VERSION_NAMESPACE(v2)
{
	namespace adaptor
	{
		template<size_t I, typename Tuple>
		decltype(auto) get_tuple_element(Tuple& tuple)
		{
			return std::get<I>(tuple).second;
		}

		template <typename Tuple, size_t ... Is>
		auto make_define_array_from_tuple_impl(Tuple& tuple, std::index_sequence<Is...>)
		{
			return v1::type::make_define_array(get_tuple_element<Is>(tuple)...);
		}

		template <typename Tuple>
		auto make_define_array_from_tuple(Tuple& tuple)
		{
			using indices_type = std::make_index_sequence<std::tuple_size<Tuple>::value>;
			return make_define_array_from_tuple_impl(tuple, indices_type{});
		}

		template <typename T>
		struct convert<T, std::enable_if_t<timax::rpc::has_meta_macro<T>::value>>
		{
			msgpack::object const& operator()(msgpack::object const& o, T& v) const
			{
				make_define_array_from_tuple(v.Meta()).msgpack_unpack(o.convert());
				return o;
			}
		};

		template <typename T>
		struct pack<T, std::enable_if_t<timax::rpc::has_meta_macro<T>::value>>
		{
			template <typename Stream>
			msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, T const& v) const
			{
				make_define_array_from_tuple(v.Meta()).msgpack_pack(o);
				return o;
			}
		};
	}
} }

namespace timax { namespace rpc 
{
	struct blob_t
	{
		blob_t() : raw_ref_() {}
		blob_t(char const* data, size_t size)
			: raw_ref_(data, static_cast<uint32_t>(size))
		{
		}

		template <typename Packer>
		void msgpack_pack(Packer& pk) const
		{
			pk.pack_bin(raw_ref_.size);
			pk.pack_bin_body(raw_ref_.ptr, raw_ref_.size);
		}

		void msgpack_unpack(msgpack::object const& o)
		{
			msgpack::operator >> (o, raw_ref_);
		}

		auto data() const
		{
			return raw_ref_.ptr;
		}

		size_t size() const
		{
			return raw_ref_.size;
		}

		msgpack::type::raw_ref	raw_ref_;
	};

	struct msgpack_codec
	{
		//using buffer_type = msgpack::sbuffer;
		using buffer_type = std::vector<char>;
		class buffer_t
		{
		public:
			buffer_t()
				: buffer_t(0)
			{ }

			explicit buffer_t(size_t len)
				: buffer_(len, 0)
				, offset_(0)
			{ }

			buffer_t(buffer_t const&) = default;
			buffer_t(buffer_t &&) = default;
			buffer_t& operator= (buffer_t const&) = default;
			buffer_t& operator= (buffer_t &&) = default;

			void write(char const* data, size_t length)
			{
				if (buffer_.size() - offset_ < length)
					buffer_.resize(length + offset_);

				std::memcpy(buffer_.data() + offset_, data, length);
				offset_ += length;
			}

			std::vector<char> release() const noexcept
			{
				return std::move(buffer_);
			}

		private:
			std::vector<char>		buffer_;
			size_t				offset_;
		};

		template <typename ... Args>
		buffer_type pack_args(Args&& ... args) const
		{
			buffer_t buffer;
			auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
			msgpack::pack(buffer, args_tuple);
			return buffer.release();
		}

		template <typename T>
		buffer_type pack(T&& t) const
		{
			buffer_t buffer;
			msgpack::pack(buffer, std::forward<T>(t));
			return buffer.release();
		}

		template <typename T>
		T unpack(char const* data, size_t length)
		{
			try
			{
				msgpack::unpack(&msg_, data, length);
				return msg_.get().as<T>();
			}
			catch (...)
			{
				using namespace std::string_literals;
				exception error{ error_code::FAIL, "Args not match!"s };
				throw error;
			}
		}

	private:
		msgpack::unpacked msg_;
	};
} }
