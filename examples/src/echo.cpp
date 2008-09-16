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

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>

// I like to have an independent error log file to keep track of exceptions while debugging.
// You might want a different filename. I just picked this because everything has access there.
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

// Let's make our request handling class. It must do the following:
// 1) Be derived from Fastcgipp::Request
// 2) Define the virtual response() member function from Fastcgipp::Request()

// First things first let's decide on what kind of character set we will use.
// Since we want to be able to echo all languages we will use unicode. The way this
// library handles unicode might be different than some are used to but it is done
// the way it is supposed to be. All internal characters are wide. In this case UTF-32.
// This way we don't have to mess around with variable size characters in our program.
// A string with 10 wchar_ts is ten characters long. Not up in the air as it is with UTF-8.
// Anyway, moving right along, the streams will code convert all the UTF-32 data to UTF-8
// before it is sent out to the client. This way we get the best of both worlds.
//
// So, whenever we are going to use UTF-8, our template parameter for Fastcgipp::Request<charT>
// should be wchar_t. Keep in mind that this suddendly makes
// everything wide character and utf compatible. Including HTTP header data (cookies, urls, yada-yada).

class Echo: public Fastcgipp::Request<wchar_t>
{
	bool response()
	{
		wchar_t langString[] = { 0x0440, 0x0443, 0x0441, 0x0441, 0x043a, 0x0438, 0x0439, 0x0000 };

		// Let's make our header, note the charset=utf-8. Remember that HTTP headers
		// must be terminated with \r\n\r\n. NOT just \n\n.
		// Let's set a cookie just for fun too, in UTF-8.
		out << "Set-Cookie: lang=" << langString << '\n';
		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";

		// Now it's all stuff you should be familiar with
		out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
		out << "<title>fastcgi++: Echo in UTF-8</title></head><body>";

		// This session data structure is defined in fastcgi++/http.hpp
		out << "<h1>Session Parameters</h1>";
		out << "<p><b>Hostname:</b> " << session.host << "<br />";
		out << "<b>User Agent:</b> " << session.userAgent << "<br />";
		out << "<b>Accepted Content Types:</b> " << session.acceptContentTypes << "<br />";
		out << "<b>Accepted Languages:</b> " << session.acceptLanguages << "<br />";
		out << "<b>Accepted Characters Sets:</b> " << session.acceptCharsets << "<br />";
		out << "<b>Referer:</b> " << session.referer << "<br />";
		out << "<b>Content Type:</b> " << session.contentType << "<br />";
		out << "<b>Query String:</b> " << session.queryString << "<br />";
		out << "<b>Cookies:</b> " << session.cookies << "<br />";
		out << "<b>Root:</b> " << session.root << "<br />";
		out << "<b>Script Name:</b> " << session.scriptName << "<br />";
		out << "<b>Content Length:</b> " << session.contentLength << "<br />";
		out << "<b>Keep Alive Time:</b> " << session.keepAlive << "<br />";
		out << "<b>Server Address:</b> " << session.serverAddress << "<br />";
		out << "<b>Server Port:</b> " << session.serverPort << "<br />";
		out << "<b>Client Address:</b> " << session.remoteAddress << "<br />";
		out << "<b>Client Port:</b> " << session.remotePort << "<br />";
		out << "<b>If Modified Since:</b> " << session.ifModifiedSince << "</p>";

		//Fastcgipp::Http::Post is defined in fastcgi++/http.hpp
		out << "<h1>Post Data</h1>";
		if(session.posts.size())
			for(Fastcgipp::Http::Session<wchar_t>::Posts::iterator it=session.posts.begin(); it!=session.posts.end(); ++it)
			{
				out << "<h2>" << it->first << "</h2>";
				if(it->second.type==Fastcgipp::Http::Post<wchar_t>::form)
				{
					out << "<p><b>Type:</b> form data<br />";
					out << "<b>Value:</b> " << it->second.value << "</p>";
				}
				
				else
				{
					out << "<p><b>Type:</b> file<br />";
					// When the post type is a file, the filename is stored in Post::value;
					out << "<b>Filename:</b> " << it->second.value << "<br />";
					out << "<b>Size:</b> " << it->second.size << "<br />";
					out << "<b>Data:</b></p><pre>";
					// We will use dump to send the raw data directly to the client
					out.dump(it->second.data.get(), it->second.size);
					out << "</pre>";
				}
			}
		else
			out << "<p>No post data</p>";

		out << "</body></html>";

		// Always return true if you are done. This will let apache know we are done
		// and the manager will destroy the request and free it's resources.
		// Return false if you are not finished but want to relinquish control and
		// allow other requests to operate. You might do this after an SQL query
		// while waiting for a reply. Passing messages to requests through the
		// manager is possible but beyond the scope of this example.
		return true;
	}
};

// The main function is easy to set up
int main()
{
	try
	{
		// First we make a Fastcgipp::Manager object, with our request handling class
		// as a template parameter.
		Fastcgipp::Manager<Echo> fcgi;
		// Now just call the object handler function. It will sleep quietly when there
		// are no requests and efficiently manage them when there are many.
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
