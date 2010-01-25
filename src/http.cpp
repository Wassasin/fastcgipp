//! \file http.cpp Defines elements of the HTTP protocol
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


#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <fastcgi++/http.hpp>

#include "utf8_codecvt.hpp"

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
	else string.erase();
	return true;
}

template bool  Fastcgipp::Http::parseValue<char>(const std::basic_string<char>& name, const std::basic_string<char>& data, std::basic_string<char>& value, char fieldSep);
template bool  Fastcgipp::Http::parseValue<wchar_t>(const std::basic_string<wchar_t>& name, const std::basic_string<wchar_t>& data, std::basic_string<wchar_t>& value, wchar_t fieldSep);
template<class charT> bool Fastcgipp::Http::parseValue(const std::basic_string<charT>& name, const std::basic_string<charT>& data, std::basic_string<charT>& value, charT fieldSep)
{
	std::basic_string<charT> searchString(name);
	searchString+='=';

	size_t it=data.find(searchString);

	if(it==std::string::npos)
		return false;

	it+=searchString.size();

	size_t size=data.find(fieldSep, it);
	if(size==std::string::npos)
		size=data.size();
	size-=it;
	value.assign(data, it, size);

	return true;
}

void Fastcgipp::Http::charToString(const char* data, size_t size, std::wstring& string)
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
			cr=use_facet<codecvt<wchar_t, char, mbstate_t> >(locale(locale::classic(), new utf8CodeCvt::utf8_codecvt_facet)).in(conversionState, data, data+size, tmpData, buffer, buffer+bufferSize, it);
			string.append(buffer, it);
			size-=tmpData-data;
			data=tmpData;
		}}
		if(cr==codecvt_base::error) throw Exceptions::CodeCvt();
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

template int Fastcgipp::Http::percentEscapedToRealBytes<char>(const char* source, char* destination, size_t size);
template int Fastcgipp::Http::percentEscapedToRealBytes<wchar_t>(const wchar_t* source, wchar_t* destination, size_t size);
template<class charT> int Fastcgipp::Http::percentEscapedToRealBytes(const charT* source, charT* destination, size_t size)
{
    if (size < 1) return 0;

	unsigned int i=0;
	charT* start=destination;
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
					*destination|=((*source|0x20)-0x57)<<shift;
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

template void Fastcgipp::Http::Environment<char>::fill(const char* data, size_t size);
template void Fastcgipp::Http::Environment<wchar_t>::fill(const char* data, size_t size);
template<class charT> void Fastcgipp::Http::Environment<charT>::fill(const char* data, size_t size)
{
	using namespace std;
	using namespace boost;

	while(size)
	{{
		size_t nameSize;
		size_t valueSize;
		const char* name;
		const char* value;
		Protocol::processParamHeader(data, size, name, nameSize, value, valueSize);
		size-=value-data+valueSize;
		data=value+valueSize;

		switch(nameSize)
		{
		case 9:
			if(!memcmp(name, "HTTP_HOST", 9))
				charToString(value, valueSize, host);
			else if(!memcmp(name, "PATH_INFO", 9))
				charToString(value, valueSize, pathInfo);
			break;
		case 11:
			if(!memcmp(name, "HTTP_ACCEPT", 11))
				charToString(value, valueSize, acceptContentTypes);
			else if(!memcmp(name, "HTTP_COOKIE", 11))
				charToString(value, valueSize, cookies);
			else if(!memcmp(name, "SERVER_ADDR", 11))
				serverAddress.assign(value, value+valueSize);
			else if(!memcmp(name, "REMOTE_ADDR", 11))
				remoteAddress.assign(value, value+valueSize);
			else if(!memcmp(name, "SERVER_PORT", 11))
				serverPort=atoi(value, value+valueSize);
			else if(!memcmp(name, "REMOTE_PORT", 11))
				remotePort=atoi(value, value+valueSize);
			else if(!memcmp(name, "SCRIPT_NAME", 11))
				charToString(value, valueSize, scriptName);
			else if(!memcmp(name, "REQUEST_URI", 11))
				charToString(value, valueSize, requestUri);
			break;
		case 12:
			if(!memcmp(name, "HTTP_REFERER", 12) && valueSize)
			{
				scoped_array<char> buffer(new char[valueSize]);
				charToString(buffer.get(), percentEscapedToRealBytes(value, buffer.get(), valueSize), referer);
			}
			else if(!memcmp(name, "CONTENT_TYPE", 12))
			{
				const char* end=(char*)memchr(value, ';', valueSize);
				charToString(value, end?end-value:valueSize, contentType);
				if(end)
				{
					const char* start=(char*)memchr(end, '=', valueSize-(end-data));
					if(start)
					{
						boundarySize=valueSize-(++start-value);
						boundary.reset(new char[boundarySize]);
						memcpy(boundary.get(), start, boundarySize);
					}
				}
			}
			else if(!memcmp(name, "QUERY_STRING", 12) && valueSize)
			{
				scoped_array<char> buffer(new char[valueSize]);
				charToString(buffer.get(), percentEscapedToRealBytes(value, buffer.get(), valueSize), queryString);
			}
			break;
		case 13:
			if(!memcmp(name, "DOCUMENT_ROOT", 13))
				charToString(value, valueSize, root);
			break;
		case 14:
			if(!memcmp(name, "REQUEST_METHOD", 14))
			{
				requestMethod = HTTP_METHOD_ERROR;
				switch(valueSize)
				{
				case 3:
					if(!memcmp(value, requestMethodLabels[HTTP_METHOD_GET], 3)) requestMethod = HTTP_METHOD_GET;
					else if(!memcmp(value, requestMethodLabels[HTTP_METHOD_PUT], 3)) requestMethod = HTTP_METHOD_PUT;
					break;
				case 4:
					if(!memcmp(value, requestMethodLabels[HTTP_METHOD_HEAD], 4)) requestMethod = HTTP_METHOD_HEAD;
					else if(!memcmp(value, requestMethodLabels[HTTP_METHOD_POST], 4)) requestMethod = HTTP_METHOD_POST;
					break;
				case 5:
					if(!memcmp(value, requestMethodLabels[HTTP_METHOD_TRACE], 5)) requestMethod = HTTP_METHOD_TRACE;
					break;
				case 6:
					if(!memcmp(value, requestMethodLabels[HTTP_METHOD_DELETE], 6)) requestMethod = HTTP_METHOD_DELETE;
					break;
				case 7:
					if(!memcmp(value, requestMethodLabels[HTTP_METHOD_OPTIONS], 7)) requestMethod = HTTP_METHOD_OPTIONS;
					else if(!memcmp(value, requestMethodLabels[HTTP_METHOD_OPTIONS], 7)) requestMethod = HTTP_METHOD_CONNECT;
					break;
				}
			}
			else if(!memcmp(name, "CONTENT_LENGTH", 14))
				contentLength=atoi(value, value+valueSize);
			break;
		case 15:
			if(!memcmp(name, "HTTP_USER_AGENT", 15))
				charToString(value, valueSize, userAgent);
			else if(!memcmp(name, "HTTP_KEEP_ALIVE", 15))
				keepAlive=atoi(value, value+valueSize);
			break;
		case 18:
			if(!memcmp(name, "HTTP_IF_NONE_MATCH", 18))
				etag=atoi(value, value+valueSize);
			break;
		case 19:
			if(!memcmp(name, "HTTP_ACCEPT_CHARSET", 19))
				charToString(value, valueSize, acceptCharsets);
			break;
		case 20:
			if(!memcmp(name, "HTTP_ACCEPT_LANGUAGE", 20))
				charToString(value, valueSize, acceptLanguages);
			break;
		case 22:
			if(!memcmp(name, "HTTP_IF_MODIFIED_SINCE", 22))
			{
				stringstream dateStream;
				dateStream.write(value, valueSize);
				dateStream.imbue(locale(locale::classic(), new posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S GMT")));
				dateStream >> ifModifiedSince;
			}
			break;
		}
	}}
}

template void Fastcgipp::Http::Environment<char>::fillPosts(const char* data, size_t size);
template void Fastcgipp::Http::Environment<wchar_t>::fillPosts(const char* data, size_t size);
template<class charT> void Fastcgipp::Http::Environment<charT>::fillPosts(const char* data, size_t size)
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

template void Fastcgipp::Http::Environment<char>::fillPostsUrlEncoded(const char* data, size_t size);
template void Fastcgipp::Http::Environment<wchar_t>::fillPostsUrlEncoded(const char* data, size_t size);
template<class charT> void Fastcgipp::Http::Environment<charT>::fillPostsUrlEncoded(const char* data, size_t size)
{
    // FIXME: Assumes entire post data will be sent in one shot for this
    // encoding. I believe this to be true, but have not been able to confirm.
    std::basic_string<charT> queryString;
    boost::scoped_array<char> buffer(new char[size]);
    memcpy (buffer.get(), data, size);
	charToString (buffer.get(), size, queryString);

    doFillPostsUrlEncoded(queryString);
}

template void Fastcgipp::Http::Environment<char>::doFillPostsUrlEncoded(std::basic_string<char> &queryString);
template void Fastcgipp::Http::Environment<wchar_t>::doFillPostsUrlEncoded(std::basic_string<wchar_t> &queryString);
template<class charT> void Fastcgipp::Http::Environment<charT>::doFillPostsUrlEncoded(std::basic_string<charT> &queryString)
{
    // FIXME: Assumes entire post data will be sent in one shot for this
    // encoding. I believe this to be true, but have not been able to confirm.

    enum {KEY, VALUE};
    Fastcgipp::Http::Post<charT> post;
    post.type = Fastcgipp::Http::Post<charT>::form;

    // split up the buffer by the "&" tokenizer to the key/value pairs
    std::vector<std::basic_string<charT> > kv_pairs;
    boost::algorithm::split (kv_pairs, queryString, boost::is_any_of ("&"));

    // For each key/value pair, split it by the "=" tokenizer to get the key and
    // value. The result should be exactly two tokens (the key and the value).
    for (int i = 0; i < kv_pairs.size(); i++)
    {
        // According to this http://www.w3schools.com/TAGS/ref_urlencode.asp
        // spaces in request parameters can be replaced with + instead of being
        // URL encoded. Catch it here.
        boost::algorithm::replace_all(kv_pairs[i], "+", " ");

        std::vector<std::basic_string<charT> > kv_pair;  // One kv pair
        boost::algorithm::split (kv_pair, kv_pairs[i], boost::is_any_of ("="));
        // Check the number of tokes. Anything but 2 is bad.
        if (kv_pair.size() != 2)
        {
            return;
            //throw ();
        }
        else // Fine.
        {
            boost::scoped_array<charT> key(new charT[(kv_pair[KEY].length() + 1) * sizeof(charT)]);
            boost::scoped_array<charT> value(new charT[(kv_pair[VALUE].length() + 1) * sizeof(charT)]);

            key[percentEscapedToRealBytes(kv_pair[KEY].c_str(), key.get(), kv_pair[KEY].length())] = '\0';
            value[percentEscapedToRealBytes(kv_pair[VALUE].c_str(), value.get(), kv_pair[VALUE].length())] = '\0';

            post.value = std::basic_string<charT>(value.get());
            // trim white spaces at the beginning and end of the key and value
            std::basic_string<charT> tmp(std::basic_string<charT>(key.get()));
            boost::trim (post.value);
            boost::trim (tmp);
            posts[tmp] = post;
        }
    }
}

bool Fastcgipp::Http::SessionId::seeded=false;

Fastcgipp::Http::SessionId::SessionId()
{
	if(!seeded)
	{
		std::srand(boost::posix_time::microsec_clock::universal_time().time_of_day().fractional_seconds());
		seeded=true;
	}

	for(char* i=data; i<data+size; ++i)
		*i=char(rand()%256);
	timestamp = boost::posix_time::second_clock::universal_time();
}

template const Fastcgipp::Http::SessionId& Fastcgipp::Http::SessionId::operator=<const char>(const char* data_);
template const Fastcgipp::Http::SessionId& Fastcgipp::Http::SessionId::operator=<const wchar_t>(const wchar_t* data_);
template<class charT> const Fastcgipp::Http::SessionId& Fastcgipp::Http::SessionId::operator=(charT* data_)
{
	if(base64Decode(data_, data_+size*4/3, data)!=data+size)
		std::memset(data, 0, size);
	timestamp = boost::posix_time::second_clock::universal_time();
	return *this;
}

template bool Fastcgipp::Http::Environment<char>::requestVarExists(const std::basic_string<char>& key);
template bool Fastcgipp::Http::Environment<wchar_t>::requestVarExists(const std::basic_string<wchar_t>& key);
template<class charT> bool Fastcgipp::Http::Environment<charT>::requestVarExists(const std::basic_string<charT>& key)
{
    PostsConstIter it;

    if((it = this->posts.find(key)) == this->posts.end())
    {
        return false;
    }
    return true;
}

const char Fastcgipp::Http::base64Characters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char* Fastcgipp::Http::requestMethodLabels[]= {
	"ERROR",
	"HEAD",
	"GET",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"OPTIONS",
	"CONNECT"
};
