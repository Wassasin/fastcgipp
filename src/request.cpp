//! \file request.cpp Defines member functions for Fastcgipp::Fcgistream and Fastcgipp::Request
/***************************************************************************
* Copyright (C) 2007 Eddie Carle [eddie@erctech.org]                       *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/


#include <fastcgi++/request.hpp>
#include "utf8_codecvt.hpp"

namespace Fastcgipp
{
	template<class charT> inline std::locale makeLocale(std::locale& loc)
	{
		return loc;
	}
	
	template<> std::locale inline makeLocale<wchar_t>(std::locale& loc)
	{
		return std::locale(loc, new utf8CodeCvt::utf8_codecvt_facet);
	}
}

template int Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::emptyBuffer();
template int Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::emptyBuffer();
template <class charT, class traits>
int Fastcgipp::Fcgistream<charT, traits>::Fcgibuf::emptyBuffer()
{
	using namespace std;
	using namespace Protocol;
	char_type const* pStreamPos=this->pbase();
	while(1)
	{{
		size_t count=this->pptr()-pStreamPos;
		size_t wantedSize=count*sizeof(char_type)+dumpSize;
		if(!wantedSize)
			break;

		int remainder=wantedSize%chunkSize;
		wantedSize+=sizeof(Header)+(remainder?(chunkSize-remainder):remainder);
		if(wantedSize>numeric_limits<uint16_t>::max()) wantedSize=numeric_limits<uint16_t>::max();
		Block dataBlock(transceiver->requestWrite(wantedSize));
		dataBlock.size=(dataBlock.size/chunkSize)*chunkSize;

		mbstate_t cs = mbstate_t();
		char* toNext=dataBlock.data+sizeof(Header);

		locale loc=this->getloc();
		if(count)
		{
			if(sizeof(char_type)!=sizeof(char))
			{
				if(use_facet<codecvt<char_type, char, mbstate_t> >(loc).out(cs, pStreamPos, this->pptr(), pStreamPos, toNext, dataBlock.data+dataBlock.size, toNext)==codecvt_base::error)
				{
					pbump(-(this->pptr()-this->pbase()));
					dumpSize=0;
					dumpPtr=0;
					throw Exceptions::CodeCvt();
				}
			}
			else
			{
				size_t cnt=min(dataBlock.size-sizeof(Header), count);
				memcpy(dataBlock.data+sizeof(Header), pStreamPos, cnt);
				pStreamPos+=cnt;
				toNext+=cnt;
			}
		}

		size_t dumpedSize=min(dumpSize, static_cast<size_t>(dataBlock.data+dataBlock.size-toNext));
		memcpy(toNext, dumpPtr, dumpedSize);
		dumpPtr+=dumpedSize;
		dumpSize-=dumpedSize;
		uint16_t contentLength=toNext-dataBlock.data+dumpedSize-sizeof(Header);
		uint8_t contentRemainder=contentLength%chunkSize;
		
		Header& header=*(Header*)dataBlock.data;
		header.setVersion(Protocol::version);
		header.setType(type);
		header.setRequestId(id.fcgiId);
		header.setContentLength(contentLength);
		header.setPaddingLength(contentRemainder?(chunkSize-contentRemainder):contentRemainder);

		transceiver->secureWrite(sizeof(Header)+contentLength+header.getPaddingLength(), id, false);	
	}}
	pbump(-(this->pptr()-this->pbase()));
	return 0;
}

template std::streamsize Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::xsputn(const char_type *s, std::streamsize n);
template std::streamsize Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::xsputn(const char_type *s, std::streamsize n);
template <class charT, class traits>
std::streamsize Fastcgipp::Fcgistream<charT, traits>::Fcgibuf::xsputn(const char_type *s, std::streamsize n)
{
	std::streamsize remainder=n;
	while(remainder)
	{
		std::streamsize actual=std::min(remainder, this->epptr()-this->pptr());
		std::memcpy(this->pptr(), s, actual*sizeof(char_type));
		this->pbump(actual);
		remainder-=actual;
		if(remainder)
		{
			s+=actual;
			emptyBuffer();
		}
	}

	return n;
}

template Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::int_type Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::overflow(Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::int_type c = traits_type::eof());
template Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::int_type Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::overflow(Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::int_type c = traits_type::eof());
template <class charT, class traits>
typename Fastcgipp::Fcgistream<charT, traits>::Fcgibuf::int_type Fastcgipp::Fcgistream<charT, traits>::Fcgibuf::overflow(Fastcgipp::Fcgistream<charT, traits>::Fcgibuf::int_type c)
{
	if(emptyBuffer() < 0)
		return traits_type::eof();
	if(!traits_type::eq_int_type(c, traits_type::eof()))
		return sputc(c);
	else
		return traits_type::not_eof(c);
}

template void Fastcgipp::Request<char>::complete();
template void Fastcgipp::Request<wchar_t>::complete();
template<class charT> void Fastcgipp::Request<charT>::complete()
{
	using namespace Protocol;
	out.flush();
	err.flush();

	Block buffer(transceiver->requestWrite(sizeof(Header)+sizeof(EndRequest)));

	Header& header=*(Header*)buffer.data;
	header.setVersion(Protocol::version);
	header.setType(END_REQUEST);
	header.setRequestId(id.fcgiId);
	header.setContentLength(sizeof(EndRequest));
	header.setPaddingLength(0);
	
	EndRequest& body=*(EndRequest*)(buffer.data+sizeof(Header));
	body.setAppStatus(0);
	body.setProtocolStatus(REQUEST_COMPLETE);

	transceiver->secureWrite(sizeof(Header)+sizeof(EndRequest), id, killCon);
}

template void Fastcgipp::Fcgistream<char, std::char_traits<char> >::dump(std::basic_istream<char>& stream);
template void Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::dump(std::basic_istream<char>& stream);
template<class charT, class traits > void Fastcgipp::Fcgistream<charT, traits>::dump(std::basic_istream<char>& stream)
{
	const size_t bufferSize=32768;
	char buffer[bufferSize];

	while(stream.good())
	{
		stream.read(buffer, bufferSize);
		dump(buffer, stream.gcount());
	}
}

template bool Fastcgipp::Request<char>::handler();
template bool Fastcgipp::Request<wchar_t>::handler();
template<class charT> bool Fastcgipp::Request<charT>::handler()
{
	using namespace Protocol;
	using namespace std;

	try
	{
		if(!(role()==RESPONDER || role()==AUTHORIZER))
		{
			Block buffer(transceiver->requestWrite(sizeof(Header)+sizeof(EndRequest)));
			
			Header& header=*(Header*)buffer.data;
			header.setVersion(Protocol::version);
			header.setType(END_REQUEST);
			header.setRequestId(id.fcgiId);
			header.setContentLength(sizeof(EndRequest));
			header.setPaddingLength(0);
			
			EndRequest& body=*(EndRequest*)(buffer.data+sizeof(Header));
			body.setAppStatus(0);
			body.setProtocolStatus(UNKNOWN_ROLE);

			transceiver->secureWrite(sizeof(Header)+sizeof(EndRequest), id, killCon);
			return true;
		}

		{
			boost::lock_guard<boost::mutex> lock(messages);
			m_message=messages.front();
			messages.pop();
		}

		if(message().type==0)
		{
			const Header& header=*(Header*)message().data.get();
			const char* body=message().data.get()+sizeof(Header);
			switch(header.getType())
			{
				case PARAMS:
				{
					if(state!=PARAMS) throw Exceptions::RecordsOutOfOrder();
					if(header.getContentLength()==0) 
					{
						state=IN;
						break;
					}
					m_environment.fill(body, header.getContentLength());
					break;
				}

				case IN:
				{
					if(state!=IN) throw Exceptions::RecordsOutOfOrder();
					if(header.getContentLength()==0)
					{
						m_environment.clearPostBuffer();
						state=OUT;
						if(response())
						{
							complete();
							return true;
						}
						break;
					}

					// Process POST data based on what our incoming content type is
					{
						const char multipart[] = "multipart/form-data";
						const char urlEncoded[] = "application/x-www-form-urlencoded";

						if(equal(multipart, multipart+sizeof(multipart), m_environment.contentType.begin()))
							m_environment.fillPostsMultipart(body, header.getContentLength());

						else if(equal(urlEncoded, urlEncoded+sizeof(urlEncoded), m_environment.contentType.begin()))
							m_environment.fillPostsUrlEncoded(body, header.getContentLength());

						else
							throw Exceptions::UnknownContentType();
					}

					inHandler(header.getContentLength());
					break;
				}

				case ABORT_REQUEST:
				{
					return true;
				}
			}
		}
		else if(response())
		{
			complete();
			return true;
		}
	}
	catch(const std::exception& e)
	{
		out << "Status: 500 Internal Server Error\r\n\r\n";
		err << '"' << e.what() << '"' << " from \"" << environment().pathInfo << "\" with a " << environment().requestMethod << " request method.";
		complete();
		return true;
	}
	return false;
}

template void Fastcgipp::Request<char>::setloc(std::locale loc_);
template void Fastcgipp::Request<wchar_t>::setloc(std::locale loc_);
template<class charT> void Fastcgipp::Request<charT>::setloc(std::locale loc_)
{
	loc=makeLocale<charT>(loc_);
	out.imbue(loc);
	err.imbue(loc);
}
