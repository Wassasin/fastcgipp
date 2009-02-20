#include <fastcgi++/asqlbind.hpp>

void Fastcgipp::asqlbinder__(ASql::Error error, const boost::function<void(Fastcgipp::Message)>& callback, int type)
{
	Message message;
	message.type=type;
	message.size=sizeof(error);
	message.data.reset(new char[message.size]);
	std::memcpy(message.data.get(), &error, message.size);		

	callback(message);
}
