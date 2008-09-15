//! \file http.cpp Defines elements of the HTTP protocol
/***************************************************************************
* Copyright (C) 2007 Eddie                                                 *
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


#include <algorithm>

#include <fastcgi++/http.hpp>

void Fastcgipp::Http::Address::assign(const char* start, const char* end)
{
	data=0;
	for(int i=24; i>=0; i-=8)
	{
		char* point=(char*)memchr(start, '.', end-start);
		data|=atoi(start, end)<<i;
		if(!point || point+1>=end) break;
		start=point+1;
	}
}

template std::basic_ostream<char, std::char_traits<char> >& Fastcgipp::Http::operator<< <char, std::char_traits<char> >(std::basic_ostream<char, std::char_traits<char> >& os, const Address& address);
template std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& Fastcgipp::Http::operator<< <wchar_t, std::char_traits<wchar_t> >(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& os, const Address& address);
template<class charT, class Traits> std::basic_ostream<charT, Traits>& Fastcgipp::Http::operator<<(std::basic_ostream<charT, Traits>& os, const Address& address)
{
	using namespace std;
	if(!os.good()) return os;
	
	try
	{
		typename basic_ostream<charT, Traits>::sentry opfx(os);
		if(opfx)
		{
			streamsize fieldWidth=os.width(0);
			charT buffer[20];
			charT* bufPtr=buffer;
			locale loc(os.getloc(), new num_put<charT, charT*>);

			for(uint32_t mask=0xff000000, shift=24; mask!=0; mask>>=8, shift-=8)
			{
				bufPtr=use_facet<num_put<charT, charT*> >(loc).put(bufPtr, os, os.fill(), static_cast<long unsigned int>((address.data&mask)>>shift));
				*bufPtr++=os.widen('.');
			}
			--bufPtr;

			charT* ptr=buffer;
			ostreambuf_iterator<charT,Traits> sink(os);
			if(os.flags() & ios_base::left)
				for(int i=max(fieldWidth, bufPtr-buffer); i>0; i--)
				{
					if(ptr!=bufPtr) *sink++=*ptr++;
					else *sink++=os.fill();
				}
			else
				for(int i=fieldWidth-(bufPtr-buffer); ptr!=bufPtr;)
				{
					if(i>0) { *sink++=os.fill(); --i; }
					else *sink++=*ptr++;
				}

			if(sink.failed()) os.setstate(ios_base::failbit);
		}
	}
	catch(bad_alloc&)
	{
		ios_base::iostate exception_mask = os.exceptions();
		os.exceptions(ios_base::goodbit);
		os.setstate(ios_base::badbit);
		os.exceptions(exception_mask);
		if(exception_mask & ios_base::badbit) throw;
	}
	catch(...)
	{
		ios_base::iostate exception_mask = os.exceptions();
		os.exceptions(ios_base::goodbit);
		os.setstate(ios_base::failbit);
		os.exceptions(exception_mask);
		if(exception_mask & ios_base::failbit) throw;
	}
	return os;
}

template std::basic_istream<char, std::char_traits<char> >& Fastcgipp::Http::operator>> <char, std::char_traits<char> >(std::basic_istream<char, std::char_traits<char> >& is, Address& address);
template std::basic_istream<wchar_t, std::char_traits<wchar_t> >& Fastcgipp::Http::operator>> <wchar_t, std::char_traits<wchar_t> >(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& is, Address& address);
template<class charT, class Traits> std::basic_istream<charT, Traits>& Fastcgipp::Http::operator>>(std::basic_istream<charT, Traits>& is, Address& address)
{
	using namespace std;
	if(!is.good()) return is;

	ios_base::iostate err = ios::goodbit;
	try
	{
		typename basic_istream<charT, Traits>::sentry ipfx(is);
		if(ipfx)
		{
			uint32_t data=0;
			istreambuf_iterator<charT, Traits> it(is);
			for(int i=24; i>=0; i-=8, ++it)
			{
				uint32_t value;
				use_facet<num_get<charT, istreambuf_iterator<charT, Traits> > >(is.getloc()).get(it, istreambuf_iterator<charT, Traits>(), is, err, value);
				data|=value<<i;
				if(i && *it!=is.widen('.')) err = ios::failbit;
			}
			if(err == ios::goodbit) address=data;
			else is.setstate(err);
		}
	}
	catch(bad_alloc&)
	{
		ios_base::iostate exception_mask = is.exceptions();
		is.exceptions(ios_base::goodbit);
		is.setstate(ios_base::badbit);
		is.exceptions(exception_mask);
		if(exception_mask & ios_base::badbit) throw;
	}
	catch(...)
	{
		ios_base::iostate exception_mask = is.exceptions();
		is.exceptions(ios_base::goodbit);
		is.setstate(ios_base::failbit);
		is.exceptions(exception_mask);
		if(exception_mask & ios_base::failbit) throw;
	}

	return is;
}

template bool Fastcgipp::Http::parseXmlValue<char>(const char* const name, const char* start, const char* end, std::basic_string<char>& string);
template bool Fastcgipp::Http::parseXmlValue<wchar_t>(const char* const name, const char* start, const char* end, std::basic_string<wchar_t>& string);
template<class charT> bool Fastcgipp::Http::parseXmlValue(const char* const name, const char* start, const char* end, std::basic_string<charT>& string)
{
	using namespace std;

	size_t searchStringSize=strlen(name)+2;
	char* searchString=new char[searchStringSize+1];
	memcpy(searchString, name, searchStringSize-2);
	*(searchString+searchStringSize-2)='=';
	*(searchString+searchStringSize-1)='"';
	*(searchString+searchStringSize)='\0';

	const char* valueStart=0;

	for(; start<=end-searchStringSize; ++start)
	{
		if(valueStart && *start=='"') break;
		if(!memcmp(searchString, start, searchStringSize))
		{
			valueStart=start+searchStringSize;
			start+=searchStringSize-1;
		}
	}

	delete [] searchString;

	if(!valueStart)
		return false;

	if(start-valueStart) charToString(valueStart, start-valueStart, string);
	return true;
}

bool Fastcgipp::Http::charToString(const char* data, size_t size, std::wstring& string)
{
	const size_t bufferSize=512;
	wchar_t buffer[bufferSize];
	using namespace std;

	if(size)
	{
		codecvt_base::result cr=codecvt_base::partial;
		while(cr==codecvt_base::partial)
		{{
			wchar_t* it;
			const char* tmpData;
			mbstate_t conversionState = mbstate_t();
			cr=use_facet<codecvt<wchar_t, char, mbstate_t> >(locale("en_US.UTF-8")).in(conversionState, data, data+size, tmpData, buffer, buffer+bufferSize, it);
			string.append(buffer, it);
			size-=tmpData-data;
			data=tmpData;
		}}
		if(cr==codecvt_base::error) return false;
		return true;
	}
}

int Fastcgipp::Http::atoi(const char* start, const char* end)
{
	bool neg=false;
	if(*start=='-')
	{
		neg=false;
		++start;
	}
	int result=0;
	for(; 0x30 <= *start && *start <= 0x39 && start<end; ++start)
		result=result*10+(*start&0x0f);

	return neg?-result:result;
}

int Fastcgipp::Http::percentEscapedToRealBytes(const char* source, char* destination, size_t size)
{
	int i=0;
	char* start=destination;
	while(1)
	{
		if(*source=='%')
		{
			*destination=0;
			for(int shift=4; shift>=0; shift-=4)
			{
				if(++i==size) break;
				++source;
				if((*source|0x20) >= 'a' && (*source|0x20) <= 'f')
					*destination|=(*source|0x20)-0x57<<shift;
				else if(*source >= '0' && *source <= '9')
					*destination|=(*source&0x0f)<<shift;
			}
			++source;
			++destination;
			if(++i==size) break;
		}
		else
		{
			*destination++=*source++;
			if(++i==size) break;
		}
	}
	return destination-start;
}

template bool Fastcgipp::Http::Session<char>::fill(const char* data, size_t size);
template bool Fastcgipp::Http::Session<wchar_t>::fill(const char* data, size_t size);
template<class charT> bool Fastcgipp::Http::Session<charT>::fill(const char* data, size_t size)
{
	using namespace std;
	using namespace boost;
	
	bool status=true;

	while(size)
	{{
		size_t nameSize;
		size_t valueSize;
		const char* name;
		const char* value;
		if(!Protocol::processParamHeader(data, size, name, nameSize, value, valueSize)) return false;;
		size-=value-data+valueSize;
		data=value+valueSize;
		
		if(nameSize==9 && !memcmp(name, "HTTP_HOST", 9))
			status=charToString(value, valueSize, host);
		else if(nameSize==15 && !memcmp(name, "HTTP_USER_AGENT", 15))
			status=charToString(value, valueSize, userAgent);
		else if(nameSize==11 && !memcmp(name, "HTTP_ACCEPT", 11))
			status=charToString(value, valueSize, acceptContentTypes);
		else if(nameSize==20 && !memcmp(name, "HTTP_ACCEPT_LANGUAGE", 20))
			status=charToString(value, valueSize, acceptLanguages);
		else if(nameSize==19 && !memcmp(name, "HTTP_ACCEPT_CHARSET", 19))
			status=charToString(value, valueSize, acceptCharsets);
		else if(nameSize==12 && !memcmp(name, "HTTP_REFERER", 12) && valueSize)
		{
			scoped_array<char> buffer(new char[valueSize]);
			status=charToString(buffer.get(), percentEscapedToRealBytes(value, buffer.get(), valueSize), referer);
		}
		else if(nameSize==12 && !memcmp(name, "CONTENT_TYPE", 12))
		{
			const char* end=(char*)memchr(value, ';', valueSize);
			status=charToString(value, end?end-value:valueSize, contentType);
			if(end)
			{
				const char* start=(char*)memchr(end, '=', valueSize-(end-data));
				if(start)
				{
					boundarySize=valueSize-(++start-data);
					boundary.reset(new char[boundarySize]);
					memcpy(boundary.get(), start, boundarySize);
				}
			}
		}
		else if(nameSize==11 && !memcmp(name, "HTTP_COOKIE", 11))
			status=charToString(value, valueSize, cookies);
		else if(nameSize==13 && !memcmp(name, "DOCUMENT_ROOT", 13))
			status=charToString(value, valueSize, root);
		else if(nameSize==11 && !memcmp(name, "SCRIPT_NAME", 11))
			status=charToString(value, valueSize, scriptName);
		else if(nameSize==12 && !memcmp(name, "QUERY_STRING", 12) && valueSize)
		{
			scoped_array<char> buffer(new char[valueSize]);
			status=charToString(buffer.get(), percentEscapedToRealBytes(value, buffer.get(), valueSize), queryString);
		}
		else if(nameSize==15 && !memcmp(name, "HTTP_KEEP_ALIVE", 15))
			keepAlive=atoi(value, value+valueSize);
		else if(nameSize==14 && !memcmp(name, "CONTENT_LENGTH", 14))
			contentLength=atoi(value, value+valueSize);
		else if(nameSize==11 && !memcmp(name, "SERVER_ADDR", 11))
			serverAddress.assign(value, value+valueSize);
		else if(nameSize==11 && !memcmp(name, "REMOTE_ADDR", 11))
			remoteAddress.assign(value, value+valueSize);
		else if(nameSize==11 && !memcmp(name, "SERVER_PORT", 11))
			serverPort=atoi(value, value+valueSize);
		else if(nameSize==11 && !memcmp(name, "REMOTE_PORT", 11))
			remotePort=atoi(value, value+valueSize);
		else if(nameSize==22 && !memcmp(name, "HTTP_IF_MODIFIED_SINCE", 22))
		{
			stringstream dateStream;
			dateStream.write(value, valueSize);
			dateStream.imbue(locale(locale::classic(), new posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S GMT")));
			dateStream >> ifModifiedSince;
		}
		else if(nameSize==18 && !memcmp(name, "HTTP_IF_NONE_MATCH", 18))
			etag=atoi(value, value+valueSize);
		/*
		else
		{
			basic_string<charT> string;
			charToString(name, nameSize, string);
			charToString(value, valueSize, otherData[string]);
		}
		*/
	}}
	return status;
}

template void Fastcgipp::Http::Session<char>::fillPosts(const char* data, size_t size);
template void Fastcgipp::Http::Session<wchar_t>::fillPosts(const char* data, size_t size);
template<class charT> void Fastcgipp::Http::Session<charT>::fillPosts(const char* data, size_t size)
{
	using namespace std;
	while(1)
	{{
		size_t bufferSize=postBufferSize+size;
		char* buffer=new char[bufferSize];
		if(postBufferSize) memcpy(buffer, postBuffer.get(), postBufferSize);
		memcpy(buffer+postBufferSize, data, size);
		postBuffer.reset(buffer);
		postBufferSize=bufferSize;

		const char* end=0;
		for(const char* i=buffer+boundarySize; i<buffer+bufferSize-boundarySize; ++i)
			if(!memcmp(i, boundary.get(), boundarySize))
			{
				end=i;
				break;
			}
		
		if(!end)
			return;

		end-=4;
		const char* start=buffer+boundarySize+2;
		const char* bodyStart=start;
		for(; bodyStart<=end-4; ++bodyStart)
			if(!memcmp(bodyStart, "\r\n\r\n", 4)) break;
		bodyStart+=4;
		basic_string<charT> name;

		if(parseXmlValue("name", start, bodyStart, name))
		{
			Post<charT>& thePost=posts[name];
			if(parseXmlValue("filename", start, bodyStart, thePost.value))
			{
				thePost.type=Post<charT>::file;
				thePost.size=end-bodyStart;
				if(thePost.size)
				{
					thePost.data.reset(new char[thePost.size]);
					memcpy(thePost.data.get(), bodyStart, thePost.size);
				}
			}
			else
			{
				thePost.type=Post<charT>::form;
				charToString(bodyStart, end-bodyStart, thePost.value);
			}
		}

		bufferSize=bufferSize-(end-buffer+2);
		if(!bufferSize)
		{
			postBuffer.reset();
			return;
		}
		buffer=new char[bufferSize];
		memcpy(buffer, end+2, bufferSize);
		postBuffer.reset(buffer);
		postBufferSize=bufferSize;
		size=0;
	}}
}


