//! \file doc.hpp Documentation file for main page and tutorials
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

/*!

\version $(VERSION)
\author Eddie Carle
\date $(DATE)
\mainpage

\ref intro \n
\ref features \n
\ref overview \n
\ref dep \n
\ref installation \n
\ref tutorials

\section intro Introduction

The fastcgi++ library started out as a C++ alternative to the official FastCGI developers kit. Although the official developers kit provided some degree of C++ interface, it was very limited. The goal of this project was to provide a framework that offered all the facilities that the C++ language has to offer. Over time the scope broadened to the point that it became more than just a simple protocol library, but a platform to develop web application under C++. To the dismay of many, this library has zero support for the old CGI protocol. The consensus was that if one were to be developing web applications under C++, efficient memory management and CPU usage would be a top priority, not CGI compatibility. Effective management of simultaneous requests without the need for multiple threads is something that fastcgi++ does best. Environment data is organized into meaningful data types as opposed to a series of text strings. Internationalization and Unicode support is another top priority. The library is templated to allow internal wide character use for efficient text processing while code converting down to utf-8 upon transmission to the client.

\section features Features

	\li Support for multiple locales and characters sets including wide Unicode and utf-8
	\li Internally manages simultaneous requests instead of leaving that to the user
	\li Establishes environment data into usable data structures
	\li Implements a task manager that can not only easily communicate outside the library, but with separate threads
	\li Provides a familiar io interface by implementing it through STL iostreams
	\li Complete compliance with FastCGI protocol version 1

\section overview Overview

The fastcgi++ library is built around three classes. Fastcgipp::Manager handles all task and request management along with the communication inside and outside the library. Fastcgipp::Transceiver handles all low level socket io and maintains send/receive buffers. Fastcgipp::Request is designed to handle the individual requests themselves. The aspects of the FastCGI protocol itself are defined in the Fastcgipp::Protocol namespace.

The Fastcgipp::Request class is a pure virtual class. The class, as is, establishes and parses environment().data. Once complete it looks to user defined virtual functions for actually generating the response. A response shall be outputted by the user defined virtuals through an output stream. Once a request has control over operation it maintains it until relinquishing it. Should the user know a request will sit around waiting for data, it can return control to Fastcgipp::Manager and have a message sent back through the manager when the data is ready. The aspects of the environment().are build around the Fastcgipp::Http namespace.

Fastcgipp::Manager basically runs an endless loop (which can be terminated through POSIX signals or a function call from another thread) that passes control to requests that have a message queued or the transceiver. It is smart enough to go into a sleep mode when there are no tasks to complete or data to receive.

Fastcgipp::Transceiver's transmit half implements a cyclic buffer that can grow indefinitely to insure that operation does not halt. The send half receives full frames and passes them through Fastcgipp::Manager onto the requests. It manages all the open connections and polls them for incoming data.

\section dep Dependencies

	\li Boost C++ Libraries >1.35.0
	\li Posix compliant OS (socket stuff)

\section installation Installation

fastcgi++ installs just the same as any library out there the uses the GNU build system...

<tt>tar -xvjf fastcgi++-$(VERSION).tar.bz2\n
cd fastcgi++-$(VERSION)\n
./configure\n
make\n
make install</tt>

If you want to build the examples do...

<tt>cd examples\n
make examples</tt>

A pkg-config file will be installed so you can compile against the libraries with

<tt>g++ -o script.fcgi script.cpp `pkg-config --libs --cflags fastcgi++`</tt>

\section tutorials Tutorials

This is a collection of tutorials that should cover most aspects of the fastcgi++ library

\subpage helloWorld : A simple tutorial outputting "Hello World" in five languages using UTF-32 internally and UTF-8 externally.

\subpage echo : An example of a FastCGI application that echoes all user data and sets a cookie

\subpage showGnu : A tutorial explaining how to display images and non-html data as well as setting locales

\subpage timer : A tutorial covering the use of the task manager and threads to have requests efficiently communicate with non-fastcgi++ data.

\subpage sessions : An example of how to utilize the internal mechanism in fastcgi++ to handle HTTP sessions.

\subpage database : An example of the explains the usage of fastcgi++'s SQL facilities with a MySQL database.

*/

/*!

\page timer Delayed response

\section timerTutorial Tutorial

Our goal here will be to make a FastCGI application that responds to clients with some text, waits five seconds and then sends more. We're going to use threading and boost::asio to handle our timer. Your going to need the boost C++ libraries for this. At least version 1.35.0.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++` -lboost_system

\subsection timerError Error Logging

Our first step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

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
\endcode

\subsection timerRequest Request Handler

Now we need to write the code that actually handles the request. In this examples we still need to derive from Fastcgipp::Request and define the Fastcgipp::Request::response() function, but also some more. We're also going to need some member data to keep track of our requests execution state, a default constructor to initialize it and a global boost::asio::io_service object. In this example let's just use plain old ISO-8859-1 and pass char as the template parameter.

\code
#include <fastcgi++/request.hpp>

#include <cstring>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>

boost::asio::io_service io;

class Timer: public Fastcgipp::Request<char>
{
private:
\endcode

We'll start with our data to keep track of the execution state and a pointer for our timer object.

\code
	enum State { START, FINISH } state;
	boost::scoped_ptr<boost::asio::deadline_timer> t;
\endcode

Now we can define our response function. It is this function that is called to generate a response for the client. We'll start it off with a switch statement that tests our execution state. It isn't a good idea to define the response() function inline as it is called from numerous spots, but for the examples readability we will make an exception.

\code
	bool response()
	{
		switch(state)
		{
			case START:
			{
\endcode

First thing we'll do is output our HTTP header. Note the charset=ISO-8859-1. Remember that HTTP headers must be terminated with "\r\n\r\n". NOT just "\n\n".

\code
				out << "Content-Type: text/html; charset=ISO-8859-1\r\n\r\n";
\endcode

Some standard HTML header output

\code
				out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />";
				out << "<title>fastcgi++: Threaded Timer</title></head><body>";
\endcode

Output a message saying we are starting the timer

\code
				out << "Starting Timer...<br />";
\endcode

If we want to the client to see everything that we just outputted now instead of when the request is complete, we will have to flush the stream buffer as it's just sitting in there now.

\code
				out.flush();
\endcode

Now let's make a five second timer.

\code
				t.reset(new boost::asio::deadline_timer(io, boost::posix_time::seconds(5)));
\endcode

Now we work with our Fastcgipp::Request::callback(). It is a boost::function that takes a Fastcgipp::Message as a single argument. This callback function will pass the message on to this request thereby having Fastcgipp::Request::response() called function again. The callback function is thread safe. That means you can pass messages back to requests from other threads.

First we'll build the message we want sent back here. Normally the message would be built by whatever is calling the callback, but this is just a simple example. A type of 0 means a FastCGI record and is used internally. All other values we can use ourselves to define different message types (sql queries, file grabs, etc...). In this example we will use type=1 for timer stuff.

\code
				Fastcgipp::Message msg;
				msg.type=1;

				{
					char cString[] = "I was passed between two threads!!";
					msg.size=sizeof(cString);
					msg.data.reset(new char[sizeof(cString)]);
					std::strncpy(msg.data.get(), cString, sizeof(cString));
				}
\endcode

Now we can give our callback function to our timer.

\code
				t->async_wait(boost::bind(callback(), msg));
\endcode

We need to set our state to FINISH so that when this response is called a second time, we don't repeat this.

\code
				state=FINISH;
\endcode

Now we will return and allow the task manager to do other things (or sleep if there is nothing to do). We must return false if the request is not yet complete.

\code
				return false;
			}
\endcode

Next step, we define what's done when Fastcgipp::Request::responce() is called a second time.

\code
			case FINISH:
			{
\endcode

Whenever Fastcgipp::Request::response() is called, the Fastcgipp::Message that lead to it's calling is stored in Fastcgipp::Request::message.

\code
				out << "Timer Finished! Our message data was \"" << message.data.get() << "\"";
				out << "</body></html>";
\endcode

And we're basically done defining our response! All we need to do is return a boolean value. Always return true if you are done. This will let apache and the manager know we are done so they can destroy the request and free it's resources.

\code
				return true;
			}
		}
		return true;
	}
\endcode

Now we can define our default constructor.

\code
public:
	Timer(): state(START) {}
};
\endcode

\subsection timerManager Requests Manager

Now we need to make our main() function. In addition to creating a Fastcgipp::Manager object with the new class we made as a template parameter and calling it's handler, we also need to set up our threads and io stuff. This isn't a boost::asio tutorial, check the boost documentation for clarification on it.

\code
#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		boost::asio::io_service::work w(io);
		boost::thread t(boost::bind(&boost::asio::io_service::run, &io));
		Fastcgipp::Manager<Timer> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

\section timerCode Full Source Code

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstring>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
boost::asio::io_service io;

#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>

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

class Timer: public Fastcgipp::Request<char>
{
private:
	enum State { START, FINISH } state;

	boost::scoped_ptr<boost::asio::deadline_timer> t;

	bool response()
	{
		switch(state)
		{
			case START:
			{
				out << "Content-Type: text/html; charset=ISO-8859-1\r\n\r\n";

				out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />";
				out << "<title>fastcgi++: Threaded Timer</title></head><body>";
				
				out << "Starting Timer...<br />";

				out.flush();

				t.reset(new boost::asio::deadline_timer(io, boost::posix_time::seconds(5)));

				Fastcgipp::Message msg;
				msg.type=1;

				{
					char cString[] = "I was passed between two threads!!";
					msg.size=sizeof(cString);
					msg.data.reset(new char[sizeof(cString)]);
					std::strncpy(msg.data.get(), cString, sizeof(cString));
				}

				t->async_wait(boost::bind(callback(), msg));

				state=FINISH;

				return false;
			}
			case FINISH:
			{
				out << "Timer Finished! Our message data was \"" << message.data.get() << "\"";
				out << "</body></html>";

				return true;
			}
		}
		return true;
	}
public:
	Timer(): state(START) {}
};

int main()
{
	try
	{
		boost::asio::io_service::work w(io);
		boost::thread t(boost::bind(&boost::asio::io_service::run, &io));

		Fastcgipp::Manager<Timer> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

*/

/*!

\page showGnu Display The Gnu

\section showGnuTutorial Tutorial

Our goal here is simple and easy. All we want to do is show the gnu.png file and effectively utilize client caching.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++`

\subsection showGnuError Error Logging

Our first step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

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
\endcode

\subsection showGnuRequest Request Handler

Now we need to write the code that actually handles the request. Quite simply, all we need to do is derive from Fastcgipp::Request and define the Fastcgipp::Request::response() function. Since we're just outputting an image, we don't need to bother with Unicode and can pass char as the template parameter.

\code
#include <fastcgi++/request.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

class ShowGnu: public Fastcgipp::Request<char>
{
\endcode

Now we can define our response function. It is this function that is called to generate a response for the client. It isn't a good idea to define the response() function inline as it is called from numerous spots, but for the examples readability we will make an exception.

\code
	bool response()
	{
		using namespace std;
		using namespace boost;
\endcode

We are going to use boost::posix_time::ptime to communicate the images modification time for cache purposes.

\code
		posix_time::ptime modTime;
		int fileSize;
		int etag;
\endcode

We'll use the POSIX stat() function (man 2 stat) to get the modification time, file size and inode number.

\code
		{
			struct stat fileStat;
			stat("gnu.png", &fileStat);
			fileSize = fileStat.st_size;
			modTime = posix_time::from_time_t(fileStat.st_mtime);
\endcode

Fastcgipp::Http::Environment implements the etag variable as an integer for better processing efficiency.

\code
			etag = fileStat.st_ino;
		}
\endcode

We will need to call Fastcgipp::Request::setloc() to set a facet in our requests locale regarding how to format the date upon insertion. It needs to conform to the HTTP standard. When setting locales for the streams, make sure to use the Fastcgipp::Request::setloc() function instead of directly imbueing them. This insures that the UTF-8 code conversion still functions properly if used.

\code
		setloc(locale(loc, new posix_time::time_facet("%a, %d %b %Y %H:%M:%S GMT")));
\endcode

If the modification time of the file is older or equal to the if-modified-since value sent to us from the client and the etag matches, we don't need to send the image to them.

\code
		if(!environment().ifModifiedSince.is_not_a_date_time() && etag==environment().etag && modTime<=environment().ifModifiedSince)
		{
			out << "Status: 304 Not Modified\r\n\r\n";
			return true;
		}
\endcode

We're going to use std::fstream to read the file data.

\code
		ifstream image("gnu.png");
\endcode

Now we transmit our HTTP header containing the modification data, file size and etag value.

\code
		out << "Last-Modified: " << modTime << '\n';
		out << "Etag: " << etag << '\n';
		out << "Content-Length: " << fileSize << '\n';
		out << "Content-Type: image/png\r\n\r\n";
\endcode

Now that the header is sent, we can transmit the actual image. To send raw binary data to the client, the streams have a dump function that bypasses the stream buffer and it's code conversion. The function is overloaded to either Fastcgipp::Fcgistream::dump(std::basic_istream<char>& stream) or Fastcgipp::Fcgistream::dump(char* data, size_t size). Remember that if we are using wide characters internally, the stream converts anything sent into the stream to UTF-8 before transmitting to the client. If we want to send binary data, we definitely don't want any code conversion so that is why this function exists.

\code
		out.dump(image);
\endcode

And we're basically done defining our response! All we need to do is return a boolean value. Always return true if you are done. This will let apache and the manager know we are done so they can destroy the request and free it's resources. Return false if you are not finished but want to relinquish control and allow other requests to operate. You would do this if the request needed to wait for a message to be passed back to it through the task manager.

\code
		return true;
	}
};
\endcode

\subsection showGnuManager Requests Manager

Now we need to make our main() function. Really all one needs to do is create a Fastcgipp::Manager object with the new class we made as a template parameter, then call it's handler. Let's go one step further though and set up a try/catch loop in case we get any exceptions and log them with our error_log function.

\code
#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		Fastcgipp::Manager<ShowGnu> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

\section showGnuCode Full Source Code

\code
#include <fstream>
#include "boost/date_time/posix_time/posix_time.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>

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

class ShowGnu: public Fastcgipp::Request<char>
{
	bool response()
	{
		using namespace std;
		using namespace boost;

		posix_time::ptime modTime;
		int fileSize;
		int etag;


		{
			struct stat fileStat;
			stat("gnu.png", &fileStat);
			fileSize = fileStat.st_size;
			modTime = posix_time::from_time_t(fileStat.st_mtime);
			etag = fileStat.st_ino;
		}

		setloc(locale(loc, new posix_time::time_facet("%a, %d %b %Y %H:%M:%S GMT")));

		if(!environment().ifModifiedSince.is_not_a_date_time() && etag==environment().etag && modTime<=environment().ifModifiedSince)
		{
			out << "Status: 304 Not Modified\r\n\r\n";
			return true;
		}

		std::ifstream image("gnu.png");

		out << "Last-Modified: " << modTime << '\n';
		out << "Etag: " << etag << '\n';
		out << "Content-Length: " << fileSize << '\n';
		out << "Content-Type: image/png\r\n\r\n";

		out.dump(image);
		return true;
	}
};

int main()
{
	try
	{
		Fastcgipp::Manager<ShowGnu> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}

\endcode

*/

/*!

\page echo Echo

\section echoTutorial Tutorial

Our goal here will be to make a FastCGI application that responds to clients with an echo of all environment().data that was processed. This will include HTTP header data along with post data that was transmitted by the client. Since we want to be able to echo any alphabets, our best solution is to use UTF-32 wide characters internally and have the library code convert it to UTF-8 before sending it to the client. Your going to need the boost C++ libraries for this. At least version 1.35.0.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++`

\subsection echoError Error Handling

Our first step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

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
\endcode

\subsection echoRequest Request Handler

Now we need to write the code that actually handles the request. Quite simply, all we need to do is derive from Fastcgipp::Request and define the Fastcgipp::Request::response() function. Since we've decided to use wide Unicode characters, we need to pass wchar_t as the template parameter to the Request class as opposed to char.

\code
#include <fastcgi++/request.hpp>

class HelloWorld: public Fastcgipp::Request<wchar_t>
{
\endcode

Now we can define our response function. It is this function that is called to generate a response for the client. It isn't a good idea to define the response() function inline as it is called from numerous spots, but for the examples readability we will make an exception.

\code
	bool response()
	{
\endcode

First thing we need to do is output our HTTP header. Let us start with settting a cookie in the client using a Unicode string. Just to see how it echoes.

\code
		wchar_t langString[] = { 0x0440, 0x0443, 0x0441, 0x0441, 0x043a, 0x0438, 0x0439, 0x0000 };
		out << "Set-Cookie: lang=" << langString << '\n';
\endcode

Next of course we need to output our content type part of the header. Note the charset=utf-8. Remember that HTTP headers must be terminated with "\r\n\r\n". Not just "\n\n".

\code
		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";
\endcode

Next we'll get some initial HTML stuff out of the way

\code
		out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
		out << "<title>fastcgi++: Echo in UTF-8</title></head><body>";
\endcode

Now we are ready to start outputting environment().data. We'll start with the non-post environment().data. This data is defined and initialized in the environment().object which is of type Fastcgipp::Http::Environment.

\code
		out << "<h1>Environment Parameters</h1>";
		out << "<p><b>Hostname:</b> " << environment().host << "<br />";
		out << "<b>User Agent:</b> " << environment().userAgent << "<br />";
		out << "<b>Accepted Content Types:</b> " << environment().acceptContentTypes << "<br />";
		out << "<b>Accepted Languages:</b> " << environment().acceptLanguages << "<br />";
		out << "<b>Accepted Characters Sets:</b> " << environment().acceptCharsets << "<br />";
		out << "<b>Referer:</b> " << environment().referer << "<br />";
		out << "<b>Content Type:</b> " << environment().contentType << "<br />";
		out << "<b>Root:</b> " << environment().root << "<br />";
		out << "<b>Script Name:</b> " << environment().scriptName << "<br />";
		out << "<b>Request Method:</b> " << environment().requestMethod << "<br />";
		out << "<b>Path Info:</b> " << environment().pathInfo << "<br />";
		out << "<b>Content Length:</b> " << environment().contentLength << "<br />";
		out << "<b>Keep Alive Time:</b> " << environment().keepAlive << "<br />";
		out << "<b>Server Address:</b> " << environment().serverAddress << "<br />";
		out << "<b>Server Port:</b> " << environment().serverPort << "<br />";
		out << "<b>Client Address:</b> " << environment().remoteAddress << "<br />";
		out << "<b>Client Port:</b> " << environment().remotePort << "<br />";
		out << "<b>If Modified Since:</b> " << environment().ifModifiedSince << "</p>";
\endcode

Now let's take a look at the GET data. The GET data is stored in the associative container environment().gets which is of type std::map< std::basic_string< charT >, std::basic_string< charT > > links names to values.

\code
		out << "<h1>GET Data</h1>";
		if(environment().gets.size())
			for(Fastcgipp::Http::Environment<wchar_t>::Gets::const_iterator it=environment().gets.begin(); it!=environment().gets.end(); ++it)
				out << "<b>" << it->first << ":</b> " << it->second << "<br />";
		else
			out << "<p>No GET data</p>";
\endcode

Now let's take a look at the cookie data. The cookie data is stored in the associative container environment().cookies which is of type std::map< std::basic_string< charT >, std::basic_string< charT > > links names to values.

\code
		out << "<h1>Cookie Data</h1>";
		if(environment().cookies.size())
			for(Fastcgipp::Http::Environment<wchar_t>::Cookies::const_iterator it=environment().cookies.begin(); it!=environment().cookies.end(); ++it)
				out << "<b>" << it->first << ":</b> " << it->second << "<br />";
		else
			out << "<p>No Cookie data</p>";
\endcode

Next, we will make a little loop to output the post. The post data is stored in the associative container environment().posts of type Fastcgipp::Http::Environment::Posts linking field names to Fastcgipp::Http::Post objects.

\code
		out << "<h1>Post Data</h1>";
\endcode

If there isn't any POST data, we'll just say so

\code
		if(environment().posts.size())
			for(Fastcgipp::Http::Environment<wchar_t>::Posts::const_iterator it=environment().posts.begin(); it!=environment().posts.end(); ++it)
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
\endcode

When the post type is a file, we have some additional information

\code
					out << "<b>Filename:</b> " << it->second.filename << "<br />";
					out << "<b>Content Type:</b> " << it->second.contentType << "<br />";
					out << "<b>Size:</b> " << it->second.size << "<br />";
					out << "<b>Data:</b></p><pre>";
\endcode

We will use Fastcgipp::Fcgistream::dump to send the file raw data directly to the client

\code
					out.dump(it->second.data.get(), it->second.size);
					out << "</pre>";
				}
			}
		else
			out << "<p>No post data</p>";
\endcode

And we're basically done defining our response! All we need to do is return a boolean value. Always return true if you are done. This will let apache and the manager know we are done so they can destroy the request and free it's resources. Return false if you are not finished but want to relinquish control and allow other requests to operate. You would do this if the request needed to wait for a message to be passed back to it through the task manager.

\code
		return true;
	}
};
\endcode

\subsection echoManager Requests Manager

Now we need to make our main() function. Really all one needs to do is create a Fastcgipp::Manager object with the new class we made as a template parameter, then call it's handler. Let's go one step further though and set up a try/catch loop in case we get any exceptions and log them with our error_log function.

\code
#include <fastcgi++/manager.hpp>

int main()
{
	try
	{
		Fastcgipp::Manager<Echo> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

\section echoWorldCode Full Source Code

\code

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>

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

class Echo: public Fastcgipp::Request<wchar_t>
{
	bool response()
	{
		wchar_t langString[] = { 0x0440, 0x0443, 0x0441, 0x0441, 0x043a, 0x0438, 0x0439, 0x0000 };

		out << "Set-Cookie: lang=" << langString << '\n';
		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";

		out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
		out << "<title>fastcgi++: Echo in UTF-8</title></head><body>";

		out << "<h1>Environment Parameters</h1>";
		out << "<p><b>Hostname:</b> " << environment().host << "<br />";
		out << "<b>User Agent:</b> " << environment().userAgent << "<br />";
		out << "<b>Accepted Content Types:</b> " << environment().acceptContentTypes << "<br />";
		out << "<b>Accepted Languages:</b> " << environment().acceptLanguages << "<br />";
		out << "<b>Accepted Characters Sets:</b> " << environment().acceptCharsets << "<br />";
		out << "<b>Referer:</b> " << environment().referer << "<br />";
		out << "<b>Content Type:</b> " << environment().contentType << "<br />";
		out << "<b>Root:</b> " << environment().root << "<br />";
		out << "<b>Script Name:</b> " << environment().scriptName << "<br />";
		out << "<b>Request Method:</b> " << environment().requestMethod << "<br />";
		out << "<b>Path Info:</b> " << environment().pathInfo << "<br />";
		out << "<b>Content Length:</b> " << environment().contentLength << "<br />";
		out << "<b>Keep Alive Time:</b> " << environment().keepAlive << "<br />";
		out << "<b>Server Address:</b> " << environment().serverAddress << "<br />";
		out << "<b>Server Port:</b> " << environment().serverPort << "<br />";
		out << "<b>Client Address:</b> " << environment().remoteAddress << "<br />";
		out << "<b>Client Port:</b> " << environment().remotePort << "<br />";
		out << "<b>If Modified Since:</b> " << environment().ifModifiedSince << "</p>";

		out << "<h1>GET Data</h1>";
		if(environment().gets.size())
			for(Fastcgipp::Http::Environment<wchar_t>::Gets::const_iterator it=environment().gets.begin(); it!=environment().gets.end(); ++it)
				out << "<b>" << it->first << ":</b> " << it->second << "<br />";
		else
			out << "<p>No GET data</p>";
		
		out << "<h1>Cookie Data</h1>";
		if(environment().cookies.size())
			for(Fastcgipp::Http::Environment<wchar_t>::Cookies::const_iterator it=environment().cookies.begin(); it!=environment().cookies.end(); ++it)
				out << "<b>" << it->first << ":</b> " << it->second << "<br />";
		else
			out << "<p>No Cookie data</p>";

		out << "<h1>POST Data</h1>";
		if(environment().posts.size())
		{
			for(Fastcgipp::Http::Environment<wchar_t>::Posts::const_iterator it=environment().posts.begin(); it!=environment().posts.end(); ++it)
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
					out << "<b>Filename:</b> " << it->second.filename << "<br />";
					out << "<b>Content Type:</b> " << it->second.contentType << "<br />";
					out << "<b>Size:</b> " << it->second.size << "<br />";
					out << "<b>Data:</b></p><pre>";
					out.dump(it->second.data.get(), it->second.size);
					out << "</pre>";
				}
			}
		}
		else
			out << "<p>No POST data</p>";

		out << "</body></html>";

		return true;
	}
};

int main()
{
	try
	{
		Fastcgipp::Manager<Echo> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}

\endcode
*/

/*!

\page helloWorld Hello World in Five Languages

\section helloWorldTutorial Tutorial

Our goal here will be to make a FastCGI application that responds to clients with a "Hello World" in five different languages.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++`

\subsection helloWorldError Error Logging

Our first step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

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
\endcode

\subsection helloWorldRequest Request Handler

Now we need to write the code that actually handles the request. Quite simply, all we need to do is derive from Fastcgipp::Request and define the Fastcgipp::Request::response() function. Since we've decided to use wide Unicode characters, we need to pass wchar_t as the template parameter to the Request class as opposed to char.

\code
#include <fastcgi++/request.hpp>

class HelloWorld: public Fastcgipp::Request<wchar_t>
{
\endcode

Now we can define our response function. It is this function that is called to generate a response for the client. It isn't a good idea to define the response() function inline as it is called from numerous spots, but for the examples readability we will make an exception.

\code
	bool response()
	{
\endcode

Let's define our hello world character strings we are going to send to the client. Unfortunately C++ doesn't yet support Unicode string literals, but it is just around the corner. Obviously we could have read this data in from a UTF-8 file, but in this example I found it easier to just use these arrays.

\code
		wchar_t russian[]={ 0x041f, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442, 0x0020, 0x043c, 0x0438, 0x0440, 0x0000 };
		wchar_t chinese[]={ 0x4e16, 0x754c, 0x60a8, 0x597d, 0x0000 };
		wchar_t greek[]={ 0x0393, 0x03b5, 0x03b9, 0x03b1, 0x0020, 0x03c3, 0x03b1, 0x03c2, 0x0020, 0x03ba, 0x03cc, 0x03c3, 0x03bc, 0x03bf, 0x0000 };
		wchar_t japanese[]={ 0x4eca, 0x65e5, 0x306f, 0x4e16, 0x754c, 0x0000 };
		wchar_t runic[]={ 0x16ba, 0x16d6, 0x16da, 0x16df, 0x0020, 0x16b9, 0x16df, 0x16c9, 0x16da, 0x16de, 0x0000 };
\endcode

Any data we want to send to the client just get's inserted into the requests Fastcgipp::Fcgistream "out" stream. It works just the same as cout in almost every way. We'll start by outputting an HTTP header to the client. Note the "charset=utf-8" and keep in mind that proper HTTP headers are to be terminated with "\r\n\r\n"; not just "\n\n".

\code
		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";
\endcode

Now we're ready to insert all the HTML data into the stream.

\code
		out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
		out << "<title>fastcgi++: Hello World in UTF-8</title></head><body>";
		out << "English: Hello World<br />";
		out << "Russian: " << russian << "<br />";
		out << "Greek: " << greek << "<br />";
		out << "Chinese: " << chinese << "<br />";
		out << "Japanese: " << japanese << "<br />";
		out << "Runic English?: " << runic << "<br />";
		out << "</body></html>";
\endcode

We'll also output a little hello to the HTTP server error log just for fun as well.

\code
		err << "Hello apache error log";
\endcode

And we're basically done defining our response! All we need to do is return a boolean value. Always return true if you are done. This will let apache and the manager know we are done so they can destroy the request and free it's resources. Return false if you are not finished but want to relinquish control and allow other requests to operate. You would do this if the request needed to wait for a message to be passed back to it through the task manager.

\code
		return true;
	}
};
\endcode

\subsection helloWorldManager Requests Manager

Now we need to make our main() function. Really all one needs to do is create a Fastcgipp::Manager object with the new class we made as a template parameter, then call it's handler. Let's go one step further though and set up a try/catch loop in case we get any exceptions and log them with our error_log function.

\code
#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		Fastcgipp::Manager<HelloWorld> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

And that's it! About as simple as it gets.

\section helloWorldCode Full Source Code

\code
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>

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

class HelloWorld: public Fastcgipp::Request<wchar_t>
{
	bool response()
	{
		wchar_t russian[]={ 0x041f, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442, 0x0020, 0x043c, 0x0438, 0x0440, 0x0000 };
		wchar_t chinese[]={ 0x4e16, 0x754c, 0x60a8, 0x597d, 0x0000 };
		wchar_t greek[]={ 0x0393, 0x03b5, 0x03b9, 0x03b1, 0x0020, 0x03c3, 0x03b1, 0x03c2, 0x0020, 0x03ba, 0x03cc, 0x03c3, 0x03bc, 0x03bf, 0x0000 };
		wchar_t japanese[]={ 0x4eca, 0x65e5, 0x306f, 0x4e16, 0x754c, 0x0000 };
		wchar_t runic[]={ 0x16ba, 0x16d6, 0x16da, 0x16df, 0x0020, 0x16b9, 0x16df, 0x16c9, 0x16da, 0x16de, 0x0000 };

		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";

		out << "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
		out << "<title>fastcgi++: Hello World in UTF-8</title></head><body>";
		out << "English: Hello World<br />";
		out << "Russian: " << russian << "<br />";
		out << "Greek: " << greek << "<br />";
		out << "Chinese: " << chinese << "<br />";
		out << "Japanese: " << japanese << "<br />";
		out << "Runic English?: " << runic << "<br />";
		out << "</body></html>";

		err << "Hello apache error log";

		return true;
	}
};

int main()
{
	try
	{
		Fastcgipp::Manager<HelloWorld> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

*/

/*!

\page sessions Sessions

\section sessionsTutorial Tutorial

Our goal here will be a FastCGI application that has very simple session capabilities. We won't authenticate the creation of sessions, and the only data we will store in them is a user supplied text string. Be warned that the facilities that fastcgi++ supplies for handling sessions are quite low level to allow for maximum customizability and no overhead costs if they aren't used.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++`

\subsection sessionsError Error Logging

Our first step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

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
\endcode

\subsection sessionsRequest Request Handler

Now we need to write the code that actually handles the request. Quite simply, all we need to do is derive from Fastcgipp::Request and define the Fastcgipp::Request::response() function. For this example we'll just use ISO-8859-1 as the character set so we'll pass char as the template parameter to the Request class as opposed to wchar_t.

\code
#include <fastcgi++/request.hpp>

class SessionExample: public Fastcgipp::Request<char>
{
\endcode

The first thing we are going to do here is set up a typedef to make accessing our sessions container easier. Sessions are contained in the Fastcgipp::Http::Sessions container which in almost all ways is just an std::map. The template parameter that the class takes is any data type you want to associate with the session. Typically someone might associate a struct that contains user ids and access level with each sessions. In this example we will just use a string. The passed template type ends up as the second type in the std::pair<> of the std::map. The first type will always be a type Fastcgipp::Http::SessionId which is a class for managing session id with their expiries effectively.

\code
	typedef Fastcgipp::Http::Sessions<std::string> Sessions;
\endcode

Now let's define our actual session data in the class. We need to declare two items: a static Sessions container and a non-static iterator for the container. The iterator will obviously point to the session data pair associate with this particular request.

\code
	static Sessions sessions;
	Sessions::iterator session;
\endcode

Now we can define our response function. It is this function that is called to generate a response for the client. It isn't a good idea to define the response() function inline as it is called from numerous spots, but for the examples readability we will make an exception.

\code
	bool response()
	{
\endcode

First off let's clean up any expired sessions. Calling this function at the beginning of every request actually doesn't create all that much overhead as is explained later on.

\code
		sessions.cleanup();
\endcode

Now we need to initialize the session data member. In this example we will do this by finding a session id passed to us from the client as a cookie. You could also use get data instead of cookies obviously. Let's call our session cookie SESSIONID. Remember that our cookies/GET data is stored in a Fastcgipp::Http::Environment object called environment().

\code
		session=sessions.find(environment().findCookie("SESSIONID").data());
\endcode
	
Now the session data member will be a valid iterator to a session or it will point to end() if the client didn't provide a session id or it was invalid.

In order to get login/logout commands from the client we will use GET information. A simple ?command=login/logout system will suffice.

\code
		std::string command=environment().gets["command"];
\endcode

Now we're going to need to set our locale to output boost::date_time stuff in a proper cookie expiry way. See Fastcgipp::Request::setloc().

\code
		setloc(std::locale(loc, new boost::posix_time::time_facet("%a, %d-%b-%Y %H:%M:%S GMT")));
\endcode

Now we do what needs to be done if we received valid session data.

\code
		if(session!=sessions.end())
		{
			if(command=="logout")
			{
\endcode

If we received a logout command then we need to delete the cookie, remove the session data from the container, reset our session iterator, and then send the response.

\code
				out << "Set-Cookie: SESSIONID=deleted; expires=Thu, 01-Jan-1970 00:00:00 GMT;\n";
				sessions.erase(session);
				session=sessions.end();
				handleNoSession();
			}
			else
			{
\endcode

Otherwise, we just call Fastcgipp::Http::SessionId::refresh() on our session ID to update the expiry time, update it on the client side and then do the response.

\code
				session->first.refresh();
				out << "Set-Cookie: SESSIONID=" << session->first << "; expires=" << sessions.getExpiry(session) << '\n';
				handleSession();
			}
		}
\endcode

With no valid session id from the client it is even easier.

\code
		else
		{
			if(command=="login")
			{
\endcode

Since the client wants to make a new session, we call Fastcgipp::Http::Sessions::generate(). This function will generate a new random session id and associated the passed session data with it. Calling a stream insertion operation on Fastcgipp::Http::SessionId will output a base64 encoding of the id value. Perfect for http communications. To coordinate, constructing the class from a char* / wchar_t* will assume the string is base64 encoded and decode it accordingly.

\code
				session=sessions.generate(environment().posts["data"].value);
				out << "Set-Cookie: SESSIONID=" << session->first << "; expires=" << sessions.getExpiry(session) << '\n';
				handleSession();
			}
			else
				handleNoSession();
		}
\endcode

Now we can output some basic stuff that will be sent regardless of whether we are in a session or not.

\code
		out << "<p>There are " << sessions.size() << " sessions open</p>\n\
	</body>\n\
</html>";
\endcode

And we're basically done defining our response! All we need to do is return a boolean value. Always return true if you are done. This will let apache and the manager know we are done so they can destroy the request and free it's resources. Return false if you are not finished but want to relinquish control and allow other requests to operate. You would do this if the request needed to wait for a message to be passed back to it through the task manager.

\code
		return true;
	}
\endcode

Now we can define the functions that generate a response whether in session or not.

\code
	void handleSession()
	{
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently in a session. The session id is " << session->first << " and the session data is \"" << session->second << "\". It will expire at " << sessions.getExpiry(session) << ".</p>\n\
		<p>Click <a href='?command=logout'>here</a> to logout</p>\n";
	}

	void handleNoSession()
	{
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently not in a session.</p>\n\
		<form action='?command=login' method='post' enctype='multipart/form-data' accept-charset='ISO-8859-1'>\n\
			<div>\n\
				Text: <input type='text' name='data' value='Hola señor, usted me almacenó en una sesión' /> \n\
				<input type='submit' name='submit' value='submit' />\n\
			</div>\n\
		</form>\n";
	}
};
\endcode

Now let's initialize the static sessions object. The constructor takes to parameters. The first defines how many seconds session live for without an refresh. The second defines the minimum number of seconds that have to pass between cleanups of old sessions. This means that if the value is 30 two calls to Fastcgipp::Sessions::cleanup() in a 30 second period will only actually preform the cleanup once. We'll go 30 seconds for each in this example.

\code
Session::Sessions Session::sessions(30, 30);
\endcode

\subsection sessionsManager Requests Manager

Now we need to make our main() function. Really all one needs to do is create a Fastcgipp::Manager object with the new class we made as a template parameter, then call it's handler. Let's go one step further though and set up a try/catch loop in case we get any exceptions and log them with our error_log function.

\code
#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		Fastcgipp::Manager<SessionExample> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

And that's it! About as simple as it gets.

\section sessionsCode Full Source Code

\code
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

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

#include <fastcgi++/request.hpp>

class SessionExample: public Fastcgipp::Request<char>
{
	typedef Fastcgipp::Http::Sessions<std::string> Sessions;

	static Sessions sessions;
	Sessions::iterator session;

	bool response()
	{

		sessions.cleanup();

		session=sessions.find(environment().findCookie("SESSIONID").data());
		
		const std::string& command=environment().findGet("command");

		setloc(std::locale(loc, new boost::posix_time::time_facet("%a, %d-%b-%Y %H:%M:%S GMT")));

		if(session!=sessions.end())
		{
			if(command=="logout")
			{
				out << "Set-Cookie: SESSIONID=deleted; expires=Thu, 01-Jan-1970 00:00:00 GMT;\n";
				sessions.erase(session);
				session=sessions.end();
				handleNoSession();
			}
			else
			{
				session->first.refresh();
				out << "Set-Cookie: SESSIONID=" << session->first << "; expires=" << sessions.getExpiry(session) << '\n';
				handleSession();
			}
		}

		else
		{
			if(command=="login")
			{
				session=sessions.generate(environment().posts["data"].value);
				out << "Set-Cookie: SESSIONID=" << session->first << "; expires=" << sessions.getExpiry(session) << '\n';
				handleSession();
			}
			else
				handleNoSession();
		}

		out << "<p>There are " << sessions.size() << " sessions open</p>\n\
	</body>\n\
</html>";
	
		return true;
	}

	void handleSession()
	{
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently in a session. The session id is " << session->first << " and the session data is \"" << session->second << "\". It will expire at " << sessions.getExpiry(session) << ".</p>\n\
		<p>Click <a href='?command=logout'>here</a> to logout</p>\n";
	}

	void handleNoSession()
	{
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently not in a session.</p>\n\
		<form action='?command=login' method='post' enctype='multipart/form-data' accept-charset='ISO-8859-1'>\n\
			<div>\n\
				Text: <input type='text' name='data' value='Hola señor, usted me almacenó en una sesión' /> \n\
				<input type='submit' name='submit' value='submit' />\n\
			</div>\n\
		</form>\n";
	}
};

#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		Fastcgipp::Manager<SessionExample> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

*/

/*!
\page database Database

\section databaseTutorial Tutorial

Our goal here will be a FastCGI application that utilizes the SQL facilities of fastcgi++. This example will rely on the MySQL engine of the SQL facilities but can be easily changed to any other. We'll create a simple logging mechanism that stores a few values into a table and then pulls the results out of it.

The process that the SQL facilities use can take some getting used to as it places emphasis on a few key things that most people might not care about. For one, maintaining data types from start to finish is insured. Two, data types are standard data types, not some weird library types (unless you consider the boost::date_time stuff weird). Three, it works with data structures that you control the allocation of. This means the members can be auto-allocated should you want. Four, most importantly, queries can be done asynchronously.

In order to fulfil these requirements we lose a few things. No quick and easy queries based on textual representations of data. Those kinds of features are what we have PHP for. This means that every query has to be prepared and have appropriate data structures built for it's parameters. It was deemed that since a FastCGI script is typically contantly running the same queries over and over, they might as well be prepared.

All code and data is located in the examples directory of the tarball. You'll have to compile with: `pkg-config --libs --cflags fastcgi++`

\subsection databaseCreate Creating the database

The first thing we need to do is create a database and table to work with our script. For simplicity's sake, connect to you MySQL server and run these commands.

\verbatim
CREATE DATABASE fastcgipp;
USE fastcgipp;
CREATE TABLE logs (ipAddress INT UNSIGNED NOT NULL, timeStamp TIMESTAMP NOT NULL,
sessionId BINARY(12) NOT NULL UNIQUE, referral TEXT) CHARACTER SET="utf8";
GRANT ALL PRIVILEGES ON fastcgipp.* TO 'fcgi'@'localhost' IDENTIFIED BY 'databaseExample';
\endverbatim

Note that if your not connecting to localhost, then make sure to change the above accordingly.

\subsection databaseError Error Logging

Our next step will be setting up an error logging system. Although requests can log errors directly to the HTTP server error log, I like to have an error logging system that's separate from the library when testing applications. Let's set up a function that takes a c style string and logs it to a file with a timestamp. Since everyone has access to the /tmp directory, I set it up to send error messages to /tmp/errlog. You can change it if you want to.

\code
#include <fstream>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>

#include <asql/mysql.hpp>
#include <fastcgi++/manager.hpp>
#include <fastcgi++/request.hpp>


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
\endcode

\subsection databaseSqlSet Query Data Structure

At this point we need to define the data structures that will contain the results and parameters for our queries. In this example the insert and select queries will both deal with the same set of fields so we only need to create one structure. To make a data structure usable with the SQL facilities it must define two functions internally as explained in ASql::Data::Set. By defining these function we turn the structure into a sort of container in the eyes of the SQL facilities.

\code
struct Log
{
\endcode

First let's define our actual data elements in the structure. Unless we are fetching/sending a binary structure we must use one of the typedefed types in ASql::Data. We can use custom types like Fastcgipp::Http::Address and still have it behave like an integer because the class provides a mechanism to access the underlying integer as a reference or pointer. We us Fastcgipp::Http::SessionId as a fixed width field matching up with a plain old data structure. It will be stored in the table as raw binary data.

As you can see, one of our values has the ability to contain null values. This capability comes from the ASql::Data::Nullable template class. See also ASql::Data::NullableArray.

Note we are in a wchar_t environment(). and we are accordingly using ASql::Data::WtextN instead of ASql::Data::TextN. Since our SQL table and connection is in utf-8, all data is code converted for you upon reception.

Our reason for using Fastcgipp::Http::SessionId is merely that it provides a good fixed binary data array that happens to have iostream inserter/extractor operators to/from base64. Also the default constructor randomly generates a value.

\code
	Fastcgipp::Http::Address ipAddress;
	ASql::Data::Datetime timestamp;
	Fastcgipp::Http::SessionId sessionId;
	ASql::Data::WtextN referral;
\endcode

The SQL facilities need to be able to find out how many "elements" there are.

\code
	size_t numberOfSqlElements() const { return 4; }
\endcode

The following function provides a method of indexing the data in our structure. It communicates type information, data location, and size to the SQL facilities. For most cases simply returning the object itself will suffice. This is accomplished through the appropriate constructor in ASql::Data::Index. Exceptional cases are when a fixed length char[] is returned as that requires a size parameter and any of the templated constructors as they merely read/write raw binary data with the table based on a field length matching the types size.

The default constructor for ASql::Data::Index makes an invalid object that should be returned in the case of default. Although this won't happen if the size returned above matches below.

\code
	ASql::Data::Index getSqlIndex(const size_t index) const
	{
		switch(index)
		{
			case 0:
				return ipAddress.getInt();
			case 1:
				return timestamp;
			case 2:
				return ASql::Data::Index(sessionId);
			case 3:
				return referral;
			default:
				return ASql::Data::Index();
		}
	}
};
\endcode

\subsection databaseRequest Request Handler

Now we need to write the code that actually handles the request. This example will be a little bit more complicated than most, but for starters, let's decide on characters sets. Let's go with utf-8 with this one; so pass wchar_t as our template parameter.

\code
class Database: public Fastcgipp::Request<wchar_t>
{
\endcode

First things first, we are going to statically build our queries so that all requests can access them. Let's make some simple strings for our SQL queries.

\code
	static const char insertStatementString[];
	static const char updateStatementString[];
	static const char selectStatementString[];
\endcode

Next we'll define our SQL connection object and two SQL statement objects. We'll define them from the ASql::MySQL namespace but they can easily be redefined from another SQL engine and maintain the same interface. This way if you change SQL engines, you needn't change your code much at all. Certainly all data types will remain consistent.

\code
	static ASql::MySQL::Connection sqlConnection;
	static ASql::MySQL::Statement insertStatement;
	static ASql::MySQL::Statement updateStatement;
	static ASql::MySQL::Statement selectStatement;
\endcode

Now we need a status indication variable for the request;

\code
	enum Status { START, FETCH } status;
\endcode

We need a container to use for out Log objects.

\code
	typedef std::vector<Log> LogContainer;
	LogContainer* selectSet;
\endcode

Now we declare our response function as usual.

\code
	bool response();
\endcode

The following object handles all data associated with the actual queries. It provides a mechanism for controlling the scope of all data required to execute the SQL statements. Be sure to read the documentation associated with the class.

\code
	ASql::Query m_query;
public:
\endcode

The constructor sets our status indicator.

\code
	Database(): status(START) {}
\endcode

Here we'll declare a static function to handle initialization of static data.

\code
	static void initSql();
};
\endcode

Now we need to do some basic initialization of our static data. The strings are straightforward. The order of question marks must match exactly to the index order of the data structure they will be read from. In this case Log. For the connection and statement objects we'll call their basic constructors that don't really initialize them.

\code
const char Database::insertStatementString[] = "INSERT INTO logs (ipAddress, timeStamp, sessionId, \ 
referral) VALUE(?, ?, ?, ?)";
const char Database::updateStatementString[] = "UPDATE logs SET timeStamp=SUBTIME(timeStamp, \
'01:00:00') WHERE ipAddress=?";
const char Database::selectStatementString[] = "SELECT SQL_CALC_FOUND_ROWS ipAddress, timeStamp, \
sessionId, referral FROM logs ORDER BY timeStamp DESC LIMIT 10";

ASql::MySQL::Connection Database::sqlConnection(1);
ASql::MySQL::Statement Database::insertStatement(Database::sqlConnection);
ASql::MySQL::Statement Database::updateStatement(Database::sqlConnection);
ASql::MySQL::Statement Database::selectStatement(Database::sqlConnection);
\endcode

Now we'll declare the function to initialize our static data

\code
void Database::initSql()
{
\endcode

Since we didn't use the full constructor on our connection object, we need to actually connect it. See ASql::MySQL::Connection::connect() for details.

\code
	sqlConnection.connect("localhost", "fcgi", "databaseExample", "fastcgipp", 0, 0, 0, "utf8");
\endcode

Now we initialize our insert and select statements. We pass the init functions any Log object (default constructed will do) wrapped by a SQL::Data::Set object so it can use the derived functions to build itself around it's structure. The ASql::Data::SetBuilder class is used to turn our Log class into a Data::Set derivation. ASql::Data::IndySetBuilder turns any data type into a Data::Set of size 1. We pass a null pointer to the associated parameters/results spot if the statement doesn't actually have any. Obviously an insert will always pass a null pointer to the results set whereas a select would often have both parameters and results. See ASql::MySQL::Statement::init() for details.

\code
	const ASql::Data::SetBuilder<Log> log;
	const ASql::Data::IndySetBuilder<unsigned int> addy;
	insertStatement.init(insertStatementString, sizeof(insertStatementString), &log, 0);
	updateStatement.init(updateStatementString, sizeof(insertStatementString), &addy, 0);
	selectStatement.init(selectStatementString, sizeof(selectStatementString), 0, &log);
\endcode

By calling this function we initialize the thread/threads that will handle SQL queries.

\code
	sqlConnection.start();
}
\endcode

Now for the actual response.

\code
bool Database::response()
{
	switch(status)
	{
		case START:
		{
\endcode

Now we set up our ASql::Query object for use with the statement and run it. Again, be sure to read the doc for the class. We set the callback function to the callback object in the Fastcgipp::Request class.

\code
			selectSet=&m_query.createResults<ASql::Data::STLSetContainer<LogContainer> >();
			m_query.setCallback(boost::bind(callback(), Fastcgipp::Message(1)));
			m_query.enableRows(true);
			selectStatement.queue(m_query);
\endcode

Now we set our status to indicate we are fetching the data from the SQL source. We return false to indicate that the request is not yet complete, just relinquishing the computer.

\code
			status=FETCH;
			return false;
		}
\endcode

So now we have been called again and our query is complete, let's send'er to the client. The following should be pretty straight forward if you've done the other tutorials.

\code
		case FETCH:
		{
			out << "Content-Type: text/html; charset=utf-8\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\n\
		<title>fastcgi++: SQL Database example</title>\n\
	</head>\n\
	<body>\n\
		<h2>Showing " << selectSet->size() << " results out of " << m_query.rows() << "</h2>\n\
		<table>\n\
			<tr>\n\
				<td><b>IP Address</b></td>\n\
				<td><b>Timestamp</b></td>\n\
				<td><b>Session ID</b></td>\n\
				<td><b>Referral</b></td>\n\
			</tr>\n";
\endcode

Regarding the above, selectSet->size() will give us the number of results post LIMIT whereas *rows will tell us the results pre LIMIT due to the use of SQL_CALC_FOUND_ROWS.

Now we'll just iterate through our ASql::Data::SetContainer and show the results in a table.

\code
			for(LogContainer::iterator it(selectSet->begin()); it!=selectSet->end(); ++it)
			{
				out << "\
			<tr>\n\
				<td>" << it->ipAddress << "</td>\n\
				<td>" << it->timestamp << "</td>\n\
				<td>" << it->sessionId << "</td>\n\
				<td>" << it->referral << "</td>\n\
			</tr>\n";
			}

			out << "\
		</table>\n\
		<p><a href='database.fcgi'>Refer Me</a></p>\n\
	</body>\n\
</html>";
\endcode

So now one last thing to do in the response is log this request. The beauty of our ASql::Query is that we can queue the SQL insert statement up, return from here and have the request completed and destroyed before the SQL statement is even complete. We needn't worry about sefaulting.

So here we go, let's build a Log structure and insert it into the database. We're also going to make is so that if the referer is empty, we'll make the value is NULL instead of just an empty string. Keep in mind that the default constructor for sessionId was called and randomly generated one.

By calling reset on m_query we basically recycle it for use a second time around. Notice the call to keepAlive to ensure that the query is not cancelled when the query object goes out of scope.

\code
			m_query.reset();
			selectSet=0;

			Log& queryParameters(m_query.createParameters<ASql::Data::SetBuilder<Log> >()->data);
			m_query.keepAlive(true);
			queryParameters.ipAddress = environment().remoteAddress;
			queryParameters.timestamp = boost::posix_time::second_clock::universal_time();
			if(environment().referer.size())
				queryParameters->referral = environment().referer;
			else
				queryParameters->referral.nullness = true;
\endcode

Above we built a query object for our insert statement. Now let's build a fun little query for our update statement. This second query will take all timestamps associated with the clients address and subtract one hour from them.

\code
			ASql::Query timeUpdate;
			unsigned int& addy(timeUpdate.createParameters<ASql::Data::IndySetBuilder<unsigned int> >()->data);
			addy=environment().remoteAddress.getInt();
			timeUpdate.keepAlive(true);
\endcode

Now that we have a second query, we can put the two together into a transaction.

\code
			ASql::MySQL::Transaction transaction;
			transaction.push(m_query, insertStatement);
			transaction.push(timeUpdate, updateStatement);
			transaction.start();
\endcode

Now let's get our of here. Return a true and the request is completed and destroyed.

\code
			return true;
		}
	}
	return true;
}
\endcode


\subsection databaseManager Requests Manager

Now we need to make our main() function. Really all one needs to do is create a Fastcgipp::Manager object with the new class we made as a template parameter, then call it's handler. Let's go one step further though and set up a try/catch loop in case we get any exceptions and log them with our error_log function. As an extra this time around we will call Database::initSql() to initialize the static data.

\code
#include <fastcgi++/manager.hpp>
int main()
{
	try
	{
		Database::initSql();
		Fastcgipp::Manager<SessionExample> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode

So be it... Jedi

\section databaseCode Full Source Code

\code
#include <fstream>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>

#include <asql/mysql.hpp>
#include <fastcgi++/manager.hpp>
#include <fastcgi++/request.hpp>


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

struct Log
{
	Fastcgipp::Http::Address ipAddress;
	ASql::Data::Datetime timestamp;
	Fastcgipp::Http::SessionId sessionId;
	ASql::Data::WtextN referral;

	size_t numberOfSqlElements() const { return 4; }

	ASql::Data::Index getSqlIndex(const size_t index) const
	{
		switch(index)
		{
			case 0:
				return ipAddress.getInt();
			case 1:
				return timestamp;
			case 2:
				return ASql::Data::Index(sessionId);
			case 3:
				return referral;
			default:
				return ASql::Data::Index();
		}
	}
};

class Database: public Fastcgipp::Request<wchar_t>
{
	static const char insertStatementString[];
	static const char updateStatementString[];
	static const char selectStatementString[];

	static ASql::MySQL::Connection sqlConnection;
	static ASql::MySQL::Statement insertStatement;
	static ASql::MySQL::Statement updateStatement;
	static ASql::MySQL::Statement selectStatement;

	enum Status { START, FETCH } status;

	typedef std::vector<Log> LogContainer;
	LogContainer* selectSet;

	bool response();

	ASql::Query m_query;
public:
	Database(): status(START) {}
	static void initSql();
};

const char Database::insertStatementString[] = "INSERT INTO logs (ipAddress, timeStamp, sessionId, \ 
referral) VALUE(?, ?, ?, ?)";
const char Database::updateStatementString[] = "UPDATE logs SET timeStamp=SUBTIME(timeStamp, \
'01:00:00') WHERE ipAddress=?";
const char Database::selectStatementString[] = "SELECT SQL_CALC_FOUND_ROWS ipAddress, timeStamp, \
sessionId, referral FROM logs ORDER BY timeStamp DESC LIMIT 10";

ASql::MySQL::Connection Database::sqlConnection(1);
ASql::MySQL::Statement Database::insertStatement(Database::sqlConnection);
ASql::MySQL::Statement Database::updateStatement(Database::sqlConnection);
ASql::MySQL::Statement Database::selectStatement(Database::sqlConnection);

void Database::initSql()
{
	sqlConnection.connect("localhost", "fcgi", "databaseExample", "fastcgipp", 0, 0, 0, "utf8");
	const ASql::Data::SetBuilder<Log> log;
	const ASql::Data::IndySetBuilder<unsigned int> addy;
	insertStatement.init(insertStatementString, sizeof(insertStatementString), &log, 0);
	updateStatement.init(updateStatementString, sizeof(updateStatementString), &addy, 0);
	selectStatement.init(selectStatementString, sizeof(selectStatementString), 0, &log);
	sqlConnection.start();
}

bool Database::response()
{
	switch(status)
	{
		case START:
		{
			selectSet=&m_query.createResults<ASql::Data::STLSetContainer<LogContainer> >()->data;
			m_query.setCallback(boost::bind(callback(), Fastcgipp::Message(1)));
			m_query.enableRows();
			selectStatement.queue(m_query);
			status=FETCH;
			return false;
		}
		case FETCH:
		{
			out << "Content-Type: text/html; charset=utf-8\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\n\
		<title>fastcgi++: SQL Database example</title>\n\
	</head>\n\
	<body>\n\
		<h2>Showing " << selectSet->size() << " results out of " << m_query.rows() << "</h2>\n\
		<table>\n\
			<tr>\n\
				<td><b>IP Address</b></td>\n\
				<td><b>Timestamp</b></td>\n\
				<td><b>Session ID</b></td>\n\
				<td><b>Referral</b></td>\n\
			</tr>\n";

			for(LogContainer::iterator it(selectSet->begin()); it!=selectSet->end(); ++it)
			{
				out << "\
			<tr>\n\
				<td>" << it->ipAddress << "</td>\n\
				<td>" << it->timestamp << "</td>\n\
				<td>" << it->sessionId << "</td>\n\
				<td>" << it->referral << "</td>\n\
			</tr>\n";
			}

			out << "\
		</table>\n\
		<p><a href='database.fcgi'>Refer Me</a></p>\n\
	</body>\n\
</html>";
			
			m_query.reset();
			selectSet=0;

			Log& queryParameters(m_query.createParameters<ASql::Data::SetBuilder<Log> >()->data);
			m_query.keepAlive(true);
			queryParameters.ipAddress = environment().remoteAddress;
			queryParameters.timestamp = boost::posix_time::second_clock::universal_time();
			if(environment().referer.size())
				queryParameters.referral = environment().referer;
			else
				queryParameters.referral.nullness = true;

			ASql::Query timeUpdate;
			unsigned int& addy(timeUpdate.createParameters<ASql::Data::IndySetBuilder<unsigned int> >()->data);
			addy=environment().remoteAddress.getInt();
			timeUpdate.keepAlive(true);

			ASql::MySQL::Transaction transaction;
			transaction.push(m_query, insertStatement);
			transaction.push(timeUpdate, updateStatement);
			transaction.start();

			return true;
		}
	}
	return true;
}

int main()
{
	try
	{
		Database::initSql();
		Fastcgipp::Manager<Database> fcgi;
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
\endcode
*/
