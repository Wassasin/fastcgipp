//! \file fcgistream.hpp Defines the Fastcgipp::Fcgistream stream and stream buffer classes
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


#include <iosfwd>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/concepts.hpp>

#include <fastcgi++/protocol.hpp>
#include <fastcgi++/transceiver.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	class FcgistreamSink: public boost::iostreams::device<boost::iostreams::output, char>
	{
	private:
		Protocol::FullId m_id;
		Protocol::RecordType m_type;
		Transceiver* m_transceiver;
	public:
		std::streamsize write(const char* s, std::streamsize n);

		void set(Protocol::FullId id, Transceiver &transceiver, Protocol::RecordType type) {m_id=id, m_type=type, m_transceiver=&transceiver;}
		void dump(const char* data, size_t size) { write(data, size); }
		void dump(std::basic_istream<char>& stream);
	};

	//! Stream class for output of client data through FastCGI
	/*!
	 * This class is derived from std::basic_ostream<charT, traits>. It acts just
	 * the same as any stream does with the added feature of the dump() function.
	 *
	 * @tparam charT Character type (char or wchar_t)
	 * @tparam traits Character traits
	 */
	template <typename charT> class Fcgistream: public boost::iostreams::filtering_stream<boost::iostreams::output, charT>
	{
	public: enum OutputEncoding {NONE, HTML, URL};
	private:
		struct Encoder: public boost::iostreams::multichar_filter<boost::iostreams::output, charT>
		{
			template<typename Sink> std::streamsize write(Sink& dest, const charT* s, std::streamsize n);

			OutputEncoding m_state;
		};
		
		Encoder& m_encoder;

		FcgistreamSink& m_sink;

	public:
		Fcgistream();
		//! Arguments passed directly to FcgistreamSink::set()
		void set(Protocol::FullId id, Transceiver& transceiver, Protocol::RecordType type) { m_sink.set(id, transceiver, type); }

		void flush();
		
		//! Dumps raw data directly into the FastCGI protocol
		/*!
		 * This function exists as a mechanism to dump raw data out the stream bypassing
		 * the stream buffer or any code conversion mechanisms. If the user has any binary
		 * data to send, this is the function to do it with.
		 *
		 * @param[in] data Pointer to first byte of data to send
		 * @param[in] size Size in bytes of data to be sent
		 */
		void dump(const char* data, size_t size) { flush(); m_sink.dump(data, size); }
		//! Dumps an input stream directly into the FastCGI protocol
		/*!
		 * This function exists as a mechanism to dump a raw input stream out this stream bypassing
		 * the stream buffer or any code conversion mechanisms. Typically this would be a filestream
		 * associated with an image or something. The stream is transmitted until an EOF.
		 *
		 * @param[in] stream Reference to input stream that should be transmitted.
		 */
		void dump(std::basic_istream<char>& stream) { flush(); m_sink.dump(stream); }
	};
}
