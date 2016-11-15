#pragma once

namespace timax { namespace rpc 
{
	template <typename T>
	struct has_meta_macro
	{
	private:
		template <typename P, typename = decltype(std::declval<P>().Meta())>
		static std::true_type test(int);
		template <typename P>
		static std::false_type test(...);
		using result_type = decltype(test<T>(0));
	public:
		static constexpr bool value = result_type::value;
	};

	struct kapok_codec
	{
		template<typename T>
		T unpack(char const* data, size_t length)
		{
			kapok::DeSerializer dr;
			dr.Parse(data, length);

			T t;
			dr.Deserialize(t);
			return t;
		}

		using buffer_type = std::string;

		template <typename ... Args>
		buffer_type pack_args(Args&& ... args) const
		{
			auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
			kapok::Serializer sr;
			sr.Serialize(args_tuple);
			return sr.GetString();
		}

		template <typename T>
		buffer_type pack(T&& t) const
		{
			kapok::Serializer sr;
			sr.Serialize(std::forward<T>(t));
			return sr.GetString();
		}
	};
} }