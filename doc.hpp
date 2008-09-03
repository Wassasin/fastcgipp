/*!

\version 0.10
\author Eddie
\mainpage

\section intro Introduction

The fastcgi++ library started out as a C++ alternative to the official FastCGI developers kit. Although the official developers kit provided some degree of C++ interface, it was very limited. The goal of this project was to provide a framework that offered all the facilities that the C++ language has to offer. Over time the scope broadened to the point that it became more than just a simple protocol library, but a platform to develop web application under C++. To the dismay of many, this library has zero support for the old CGI protocol. The consensus was that if one were to be developing web applications under C++, efficient memory management and CPU usage would be a top priority, not CGI compatibility. Effective management of simultaneous requests without the need for multiple threads is something that fastcgi++ does best. Session data is organized into meaningful data types as opposed to a series of text strings. Internationalization and Unicode support is another top priority. The library is templated to allow internal wide character use for efficient text processing while code converting down to utf-8 upon transmission to the client.

\section features Features

	\li Support for multiple locales and characters sets including wide Unicode and utf-8
	\li Internally manages simultaneous requests instead of leaving that to the user
	\li Establishes session data into usable data structures
	\li Implements a task manager that can not only easily communicate outside the library, but with separate threads
	\li Provides a familiar io interface by implementing it through STL iostreams
	\li Complete compliance with FastCGI protocol version 1

\section overview Overview

The fastcgi++ library is built around three classes. Fastcgipp::Manager handles all task and request management along with the communication inside and outside the library. Fastcgipp::Transceiver handles all low level socket io and maintains send/receive buffers. Fastcgipp::Request is designed to handle the individual requests themselves. The aspects of the FastCGI protocol itself are defined in the Fastcgipp::Protocol namespace.

The Fastcgipp::Request class is a pure virtual class. The class, as is, establishes and parses session data. Once complete it looks to user defined virtual functions for actually generating the response. A response shall be outputted by the user defined virtuals through an output stream. Once a request has control over operation it maintains it until relinquishing it. Should the user know a request will sit around waiting for data, it can return control to Fastcgipp::Manager and have a message sent back through the manager when the data is ready. The aspects of the session are build around the Fastcgipp::Http namespace.

Fastcgipp::Manager basically runs an endless loop (which can be terminated through POSIX signals or a function call from another thread) that passes control to requests that have a message queued or the transceiver. It is smart enough to go into a sleep mode when there are no tasks to complete or data to receive.

Fastcgipp::Transceiver's transmit half implements a cyclic buffer that can grow indefinitely to insure that operation does not halt. The send half receives full frames and passes them through Fastcgipp::Manager onto the requests. It manages all the open connections and polls them for incoming data.

\section tutorials Tutorials

This is a collection of tutorials that should cover most aspects of the fastcgi++ library

\subpage helloWorld : A simple tutorial outputting "Hello World" in five languages using UTF-32 internally and UTF-8 externally.


\page helloWorld Hello World in Five Languages

\section helloWorldTutorial Tutorial

Our goal here will be to make a FastCGI application that responds to clients with a "Hello World" in five different languages. Since we'll need so many different alphabets, our best solution is to use UTF-32 wide characters internally and have the library code convert it to UTF-8 before sending it to the client. Your going to need the boost C++ libraries for this. At least version 1.35.0.

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

Now we can define our response function. It is this function that is called to generate a response for the client.

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

We are already set up to deal with wide characters internally, but in order to make the library (specifically the stream buffer) code convert the wide characters to UTF-8, we need to set up a proper UTF-8 locale. I've picked "en_US.UTF-8" because it is the most common, but for different primary languages and localities you might want to pick something different eg. "ru_RU.UTF-8". You could simply call out.imbue(), but since we have two streams (err and our) and a locale object stored in the class for general reference, the request class implements a member function to change all locales simultaneously. If you don't have any UTF-8 locales installed on your system, edit /etc/locale.gen, add them and then run locale-gen.

\code
		setloc(std::locale("en_US.UTF-8"));
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

		setloc(std::locale("en_US.UTF-8"));

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
