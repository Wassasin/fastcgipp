//! \file request.hpp Defines the Fastcgipp::Request class
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


#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <queue>
#include <map>
#include <string>
#include <locale>

#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>

#include <fastcgi++/transceiver.hpp>
#include <fastcgi++/protocol.hpp>
#include <fastcgi++/exceptions.hpp>
#include <fastcgi++/fcgistream.hpp>
#include <fastcgi++/http.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	template<class T> class Manager;

	//! %Request handling class
	/*!
	 * Derivations of this class will handle requests. This
	 * includes building the environment data, processing post/get data,
	 * fetching data (files, database), and producing a response.
	 * Once all client data is organized, response() will be called.
	 * At minimum, derivations of this class must define response().
	 *
	 * If you want to use UTF-8 encoding pass wchar_t as the template
	 * argument, use setloc() to setup a UTF-8 locale and use wide 
	 * character unicode internally for everything. If you want to use
	 * a 8bit character set encoding pass char as the template argument and
	 * setloc() a locale with the corresponding character set.
	 *
	 * \tparam charT Character type for internal processing (wchar_t or char)
	 */
	template<class charT> class Request
	{
	public:
		//! Initializes what it can. set() must be called by Manager before the data is usable.
		/*!
		 * \param maxPostSize This would be the maximum size you want to allow for
		 * post data. Any data beyond this size would result in a call to
		 * bigPostErrorHandler(). A value of 0 represents unlimited.
		 */
		Request(const size_t maxPostSize=0): m_maxPostSize(maxPostSize), state(Protocol::PARAMS)  { setloc(std::locale::classic()); out.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit); m_environment.clearPostBuffer(); }

		//! Accessor for  the data structure containing all HTTP environment data
		const Http::Environment<charT>& environment() const { return m_environment; }

		// To dump data into the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump(char* data, size_t size)
		// or Fcgistream::dump(std::basic_istream<char>& stream)
		
		//! Standard output stream to the client
		/*!
		 * To dump data directly through the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump()
		 */
		Fcgistream<charT> out;

		//! Output stream to the HTTP server error log
		/*!
		 * To dump data directly through the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump()
		 */
		Fcgistream<charT> err;

		//! Called when an exception is caught.
		/*!
		 * This function is called whenever an exception is caught inside the request. By default it will output some data
		 * to the error log and send a standard 500 Internal Server Error message to the user. Override for more specialized
		 * purposes.
		 *
		 * @param[in] error Exception caught
		 */
		virtual void errorHandler(const std::exception& error);

		//! Called when too much post data is recieved.
		virtual void bigPostErrorHandler();

		//! See the requests role
		Protocol::Role role() const { return m_role; }

		//! Accessor for the callback function for dealings outside the fastcgi++ library
		/*!
		 * The purpose of the callback object is to provide a thread safe mechanism for functions and
		 * classes outside the fastcgi++ library to talk to the requests. Should the library
		 * wish to have another thread process or fetch some data, that thread can call this
		 * function when it is finished. It is equivalent to this:
		 *
		 * void callback(Message msg);
		 *
		 *	The sole parameter is a Message that contains both a type value for processing by response()
		 *	and the raw castable data.
		 */
		const boost::function<void(Message)>& callback() const { return m_callback; }

		//! Set the requests locale
		/*!
		 * This function both sets loc to the locale passed to it and imbues the locale into the
		 * out and err stream. The user should always call this function as opposed to setting the
		 * locales directly is this functions insures the utf8 code conversion is functioning properly.
		 *
		 * @param[in] loc_ New locale
		 * @sa loc
		 * @sa out
		 */
		void setloc(std::locale loc_){ out.imbue(loc_); err.imbue(loc_); }

		//! Retrieves the requests locale
		/*!
		 * \return Constant reference to the requests locale
		 */
		const std::locale& getloc(){ return loc; }

	protected:
		//! Response generator
		/*!
		 * This function is called by handler() once all request data has been received from the other side or if a
		 * Message not of a FastCGI type has been passed to it. The function shall return true if it has completed
		 * the response and false if it has not (waiting for a callback message to be sent).
		 *
		 * @return Boolean value indication completion (true means complete)
		 * @sa callback
		 */
		virtual bool response() =0;

		//! Generate a data input response
		/*!
		 * This function exists should the library user wish to do something like generate a partial response based on
		 * bytes received from the client. The function is called by handler() every time a FastCGI IN record is received.
		 * The function has no access to the data, but knows exactly how much was received based on the value that was passed.
		 * Note this value represents the amount of data received in the individual record, not the total amount received in
		 * the environment. If the library user wishes to have such a value they would have to keep a tally of all size values
		 * passed.
		 *
		 * @param[in] bytesReceived Amount of bytes received in this FastCGI record
		 */
		virtual void inHandler(int bytesReceived) { };

		//! Process custom POST data
		/*!
		 * Override this function should you wish to process non-standard post
		 * data. The library will on it's own process post data of the types
		 * "multipart/form-data" and "application/x-www-form-urlencoded". To use
		 * this function, your raw post data is fully assembled in
		 * environment().postBuffer(), the size is in environment().contentLength,
		 * and the type string is stored in environment().contentType. Should the
		 * content type be what you're looking for and you've processed it, simply
		 * return true. Otherwise return false.  Do not worry about freeing the
		 * data in the post buffer. Should you return false, the system will try
		 * to internally process it.
		 *
		 * @return Return true if you've processed the data.
		 */
		bool virtual inProcessor() { return false; }

		//! The message associated with the current handler() call.
		/*!
		 * This is only of use to the library user when a non FastCGI (type=0) Message is passed
		 * by using the requests callback.
		 *
		 * @sa callback
		 */
		const Message& message() const { return m_message; }

	private:
		template<class T> friend class GenManager;

		//! The locale associated with the request. Should be set with setloc(), not directly.
		std::locale loc;

		//! The message associated with the current handler() call.
		/*!
		 * This is only of use to the library user when a non FastCGI (type=0) Message is passed
		 * by using the requests callback.
		 *
		 * @sa callback
		 */
		Message m_message;

		//! The callback function for dealings outside the fastcgi++ library
		/*!
		 * The purpose of the callback object is to provide a thread safe mechanism for functions and
		 * classes outside the fastcgi++ library to talk to the requests. Should the library
		 * wish to have another thread process or fetch some data, that thread can call this
		 * function when it is finished. It is equivalent to this:
		 *
		 * void callback(Message msg);
		 *
		 *	The sole parameter is a Message that contains both a type value for processing by response()
		 *	and the raw castable data.
		 */
		boost::function<void(Message)> m_callback;

		//! The data structure containing all HTTP environment data
		Http::Environment<charT> m_environment;

		//! Queue type for pending messages
		/*!
		 * This is merely a derivation of a std::queue<Message> and a
		 * boost::mutex that gives data locking abilities to the STL container.
		 */
		class Messages: public std::queue<Message>, public boost::mutex {};
		//! A queue of messages to be handler by the request
		Messages messages;

		//! The maximum amount of post data that can be recieved
		const size_t m_maxPostSize;

		//! Request Handler
		/*!
		 * This function is called by Manager::handler() to handle messages destined for the request.
		 * It deals with FastCGI messages (type=0) while passing all other messages off to response().
		 *
		 * @return Boolean value indicating completion (true means complete)
		 * @sa callback
		 */
		bool handler();
		//! Pointer to the transceiver object that will send data to the other side
		Transceiver* transceiver;
		//! The role that the other side expects this request to play
		Protocol::Role m_role;
		//! The complete ID (request id & file descriptor) associated with the request
		Protocol::FullId id;
		//! Boolean value indicating whether or not the file descriptor should be closed upon completion.
		bool killCon;
		//! What the request is current doing
		Protocol::RecordType state;
		//! Generates an END_REQUEST FastCGI record
		void complete();
		//! Set's up the request with the data it needs.
		/*!
		 * This function is an "after-the-fact" constructor that build vital initial data for the request.
		 *
		 * @param[in] id_ Complete ID of the request
		 * @param[in] transceiver_ Transceiver object the request will use
		 * @param[in] role_ The role that the other side expects this request to play
		 * @param[in] killCon_ Boolean value indicating whether or not the file descriptor should be closed upon completion
		 * @param[in] callback_ Callback function capable of passing messages to the request
		 */
		void set(Protocol::FullId id_, Transceiver& transceiver_, Protocol::Role role_, bool killCon_, boost::function<void(Message)> callback_)
		{
			killCon=killCon_;
			id=id_;
			transceiver=&transceiver_;
			m_role=role_;
			m_callback=callback_;

			err.set(id_, transceiver_, Protocol::ERR);
			out.set(id_, transceiver_, Protocol::OUT);
		}
	};

	//! Includes all exceptions used by the fastcgi++ library
	namespace Exceptions
	{
		/** 
		 * @brief Thrown if FastCGI records are received out of order.
		 */
		struct RecordsOutOfOrder: public std::exception
		{
			const char* what() const throw() { return "FastCGI records received out of order from server."; }
		};

		/** 
		 * @brief Thrown if a incoming content type is unknown
		 */
		struct UnknownContentType: public std::exception
		{
			const char* what() const throw() { return "Client sent unknown content type."; }
		};
	}
}

#endif
