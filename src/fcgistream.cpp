#include <cstring>
#include <algorithm>
#include <boost/iostreams/code_converter.hpp>

#include "fastcgi++/fcgistream.hpp"
#include "utf8_codecvt.hpp"

template<typename charT> template<typename Sink> std::streamsize Fastcgipp::Fcgistream<charT>::Encoder::write(Sink& dest, const charT* s, std::streamsize n)
{
	switch(m_state)
	{
		case NONE:
		{
			boost::iostreams::write(dest, s, n);
			return n;
		}
	}
}

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
void error_log(const char* msg)
{
	using namespace std;
	using namespace boost;
	static ofstream error;
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << msg << endl;
}
std::streamsize Fastcgipp::FcgistreamSink::write(const char* s, std::streamsize n)
{
	using namespace std;
	using namespace Protocol;
	const std::streamsize totalUsed=n;
	while(1)
	{{
		if(!n)
			break;

		int remainder=n%chunkSize;
		size_t size=n+sizeof(Header)+(remainder?(chunkSize-remainder):remainder);
		if(size>numeric_limits<uint16_t>::max()) size=numeric_limits<uint16_t>::max();
		Block dataBlock(m_transceiver->requestWrite(size));
		size=(dataBlock.size/chunkSize)*chunkSize;

		uint16_t contentLength=std::min(size-sizeof(Header), size_t(n));
		memcpy(dataBlock.data+sizeof(Header), s, contentLength);

		std::stringstream ss;
		ss << "Sending the " << contentLength << " byte chunk '";
		ss.write(s, contentLength);
		ss << "'";
		error_log(ss.str().c_str());

		s+=contentLength;
		n-=contentLength;


		uint8_t contentPadding=chunkSize-contentLength%chunkSize;
		if(contentPadding==8) contentPadding=0;
		
		Header& header=*(Header*)dataBlock.data;
		header.setVersion(Protocol::version);
		header.setType(m_type);
		header.setRequestId(m_id.fcgiId);
		header.setContentLength(contentLength);
		header.setPaddingLength(contentPadding);

		m_transceiver->secureWrite(size, m_id, false);	
	}}
	return totalUsed;
}

void Fastcgipp::FcgistreamSink::dump(std::basic_istream<char>& stream)
{
	const size_t bufferSize=32768;
	char buffer[bufferSize];

	while(stream.good())
	{
		stream.read(buffer, bufferSize);
		write(buffer, stream.gcount());
	}
}

template<typename T, typename toChar, typename fromChar> T& fixPush(boost::iostreams::filtering_stream<boost::iostreams::output, fromChar>& stream, const T& t, std::streamsize buffer_size)
{
	stream.push(t, buffer_size);
	return *stream.template component<T>(stream.size()-1);
}

template<> Fastcgipp::FcgistreamSink& fixPush<Fastcgipp::FcgistreamSink, char, wchar_t>(boost::iostreams::filtering_stream<boost::iostreams::output, wchar_t>& stream, const Fastcgipp::FcgistreamSink& t, std::streamsize buffer_size)
{
	stream.push(boost::iostreams::code_converter<Fastcgipp::FcgistreamSink, utf8CodeCvt::utf8_codecvt_facet>(t, buffer_size));
	return **stream.component<boost::iostreams::code_converter<Fastcgipp::FcgistreamSink, utf8CodeCvt::utf8_codecvt_facet> >(stream.size()-1);
}


template Fastcgipp::Fcgistream<char>::Fcgistream();
template Fastcgipp::Fcgistream<wchar_t>::Fcgistream();
template<typename charT> Fastcgipp::Fcgistream<charT>::Fcgistream():
	//m_encoder(fixPush<Encoder, charT, charT>(*this, Encoder(), 0)),
	m_sink(fixPush<FcgistreamSink, char, charT>(*this, FcgistreamSink(), 8192))
{}

template void Fastcgipp::Fcgistream<char>::flush();
template void Fastcgipp::Fcgistream<wchar_t>::flush();
template<typename charT> void Fastcgipp::Fcgistream<charT>::flush()
{
	error_log("Calling sync");
	boost::iostreams::filtering_stream<boost::iostreams::output, charT>::strict_sync();
}
