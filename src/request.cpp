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

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
void errorlog(const char* msg)
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
template void Fastcgipp::Request<char>::complete();
template void Fastcgipp::Request<wchar_t>::complete();
template<class charT> void Fastcgipp::Request<charT>::complete()
{
	using namespace Protocol;
	out.flush();
	err.flush();
	errorlog("Closing up shop");

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
		errorHandler(e);
		complete();
		return true;
	}
	return false;
}

template void Fastcgipp::Request<char>::errorHandler(const std::exception& error);
template void Fastcgipp::Request<wchar_t>::errorHandler(const std::exception& error);
template<class charT> void Fastcgipp::Request<charT>::errorHandler(const std::exception& error)
{
		out << \
"Status: 500 Internal Server Error\n"\
"Content-Type: text/html; charset=ISO-8859-1\r\n\r\n"\
"<!DOCTYPE html>"\
"<html lang='en'>"\
	"<head>"\
		"<title>500 Internal Server Error</title>"\
	"</head>"\
	"<body>"\
		"<h1>500 Internal Server Error</h1>"\
	"</body>"\
"</html>";

		err << '"' << error.what() << '"' << " from \"http://" << environment().host << environment().requestUri << "\" with a " << environment().requestMethod << " request method.";
}
