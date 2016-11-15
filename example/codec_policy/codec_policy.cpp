#include <rest_rpc/rpc.hpp>

struct foo_t
{
	int			a;
	double		b;
	std::string	c;

	//template <typename Packer> 
	//void msgpack_pack(Packer& pk) const 
	//{
	//	msgpack::type::make_define_array(a, b, c).msgpack_pack(pk);
	//}
	//
	//void   (msgpack::object const& o)
	//{
	//	msgpack::type::make_define_array(a, b, c).msgpack_unpack(o);
	//}
	//
	//template <typename MSGPACK_OBJECT>
	//void msgpack_object(MSGPACK_OBJECT* o, msgpack::zone& z) const
	//{
	//	msgpack::type::make_define_array(a, b, c).msgpack_object(o, z);
	//}

	//MSGPACK_DEFINE(a, b, c);
	META(a, b, c);
};

// msgpack adapt META macro
int main()
{
	foo_t foo = { 1, 2.0, "ryuga waga teki wo kuraou!" };
	msgpack::sbuffer buffer;
	msgpack::pack(buffer, foo);
	
	msgpack::unpacked msg;
	msgpack::unpack(&msg, buffer.data(), buffer.size());
	auto foo1 = msg.get().as<foo_t>();
	
	std::cout << foo1.a << " | " << foo1.b << " | " << foo1.c << std::endl;

	return 0;
}
