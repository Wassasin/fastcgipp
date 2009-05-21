//! \file asql.hpp Declares the ASql namespace
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

#ifndef ASQL_HPP
#define ASQL_HPP

#include <vector>
#include <queue>
#include <cstring>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>


//! Defines classes and functions relating to SQL querying
namespace ASql
{
	/** 
	 * @brief SQL Error
	 */
	struct Error: public std::exception
	{
		/** 
		 * @brief Associated error number.
		 */
		int erno;
		/** 
		 * @brief Pointer to string data explaining error.
		 */
		const char* msg;
		/** 
		 * @param[in] msg_ Pointer to string explaining error.
		 * @param[in] erno_ Associated error number.
		 */
		Error(const char* msg_, const int erno_): erno(erno_), msg(msg_) {}
		Error(): erno(0), msg(0) {}
		const char* what() const throw() { return msg; }

		Error(const Error& e): erno(e.erno), msg(e.msg)  {}
	};

	//! Defines data types and conversion techniques standard to the fastcgipp %SQL facilities.
	namespace Data
	{
		/** 
		 * @brief Defines data types supported by the fastcgi++ sql facilities.
		 *
		 * This enumeration provides runtime type identification capabilities to classes derived from
		 * the Set class. All types starting with U_ mean unsigned and all types ending will _N means
		 * they can store null values via the Nullable class.
		 */
		enum Type { U_TINY=0,
						U_SHORT,
						U_INT,
						U_BIGINT,
						TINY,
						SHORT,
						INT,
						BIGINT,
						FLOAT,
						DOUBLE,
						TIME,
						DATE,
						DATETIME,
						BLOB,
						TEXT,
						WTEXT,
						CHAR,
						BINARY,
						BIT,		
						U_TINY_N,
						U_SHORT_N,
						U_INT_N,
						U_BIGINT_N,
						TINY_N,
						SHORT_N,
						INT_N,
						BIGINT_N,
						FLOAT_N,
						DOUBLE_N,
						TIME_N,
						DATE_N,
						DATETIME_N,
						BLOB_N,
						TEXT_N,
						WTEXT_N,
						CHAR_N,
						BINARY_N,
						BIT_N,
						NOTHING };
		
		/** 
		 * @brief Base class to the Nullable template class.
		 *
		 * This base class provides a polymorphic method of retrieving a void pointer to the contained
		 * object regardless of it's type along with it's nullness.
		 *
		 * If nullness is true then the value is null.
		 */
		struct NullablePar
		{
			NullablePar(bool _nullness): nullness(_nullness) { }
			bool nullness;
			/** 
			 * @brief Retrieve a void pointer to the object contained in the class.
			 * 
			 * @return Void pointer to the object contained in the class.
			 */
			virtual void* getVoid() =0;
		};
		
		/** 
		 * @brief Class for adding null capabilities to any type. Needed for SQL queries involving
		 * null values.
		 */
		template<class T> struct Nullable: public NullablePar
		{
			T object;
			void* getVoid() { return &object; }
			operator T() { return object; }
			operator const T() const { return object; }
			Nullable(): NullablePar(false) {}
			Nullable(const T& x): NullablePar(false), object(x) { }
		};

		/** 
		 * @brief Class for adding null capabilities to character arrays.
		 */
		template<class T, int size> struct NullableArray: public NullablePar
		{
			T object[size];
			void* getVoid() { return object; }
			operator T*() { return object; }
			NullableArray(): NullablePar(false) {}
			NullableArray(const T& x): NullablePar(false), object(x) { }
		};

		/** 
		 * @brief A basic, practically none-functional stream inserter for Nullable objects.
		 */
		template<class charT, class Traits, class T> inline std::basic_ostream<charT, Traits>& operator<<(std::basic_ostream<charT, Traits>& os, const Nullable<T>& x)
		{
			if(x.nullness)
				os << "NULL";
			else
				os << x.object;

			return os;
		}

		typedef unsigned char Utiny;
		typedef signed char Tiny;
		typedef unsigned short int Ushort;
		typedef short int Short;
		typedef unsigned int Uint;
		typedef int Int;
		typedef unsigned long long int Ubigint;
		typedef long long int Bigint;
		typedef float Float;
		typedef double Double;
		typedef boost::posix_time::time_duration Time;
		typedef boost::gregorian::date Date;
		typedef boost::posix_time::ptime Datetime;
		typedef std::vector<char> Blob;
		typedef std::string Text;
		typedef std::wstring Wtext;
		//typedef std::bitset Bit;

		typedef Nullable<unsigned char> UtinyN;
		typedef Nullable<char> TinyN;
		typedef Nullable<unsigned short int> UshortN;
		typedef Nullable<short int> ShortN;
		typedef Nullable<unsigned int> UintN;
		typedef Nullable<int> IntN;
		typedef Nullable<unsigned long long int> UbigintN;
		typedef Nullable<long long int> BigintN;
		typedef Nullable<float> FloatN;
		typedef Nullable<double> DoubleN;
		typedef Nullable<boost::posix_time::time_duration> TimeN;
		typedef Nullable<boost::gregorian::date> DateN;
		typedef Nullable<boost::posix_time::ptime> DatetimeN;
		typedef Nullable<std::vector<char> > BlobN;
		typedef Nullable<std::string> TextN;
		typedef Nullable<std::wstring> WtextN;
		//typedef Nullable<std::bitset> BitN;

		/** 
		 * @brief Stores on index value from a Set
		 *
		 * All of the constructors allow for implicit construction upon return from 
		 * Set::getSqlIndex() except for the templated generic binary ones.
		 */
		struct Index
		{
			Type type;
			void* data;
			size_t size;

			Index(const Utiny& x): type(U_TINY), data(const_cast<Utiny*>(&x)) { }
			Index(const Tiny& x): type(TINY), data(const_cast<Tiny*>(&x)) { }
			Index(const Ushort& x): type(U_SHORT), data(const_cast<Ushort*>(&x)) { }
			Index(const Short& x): type(SHORT), data(const_cast<Short*>(&x)) { }
			Index(const Uint& x): type(U_INT), data(const_cast<Uint*>(&x)) { }
			Index(const Int& x): type(INT), data(const_cast<Int*>(&x)) { }
			Index(const Ubigint& x): type(U_BIGINT), data(const_cast<Ubigint*>(&x)) { }
			Index(const Bigint& x): type(BIGINT), data(const_cast<Bigint*>(&x)) { }
			Index(const Float& x): type(FLOAT), data(const_cast<Float*>(&x)) { }
			Index(const Double& x): type(DOUBLE), data(const_cast<Double*>(&x)) { }
			Index(const Time& x): type(TIME), data(const_cast<Time*>(&x)) { }
			Index(const Date& x): type(DATE), data(const_cast<Date*>(&x)) { }
			Index(const Datetime& x): type(DATETIME), data(const_cast<Datetime*>(&x)) { }
			Index(const Blob& x): type(BLOB), data(const_cast<Blob*>(&x)) { }
			Index(const Text& x): type(TEXT), data(const_cast<Text*>(&x)) { }
			Index(const Wtext& x): type(WTEXT), data(const_cast<Wtext*>(&x)) { }
			Index(const char* const x, const size_t size_): type(CHAR), data(const_cast<char*>(x)), size(size_) { }
			template<class T> explicit Index(const T& x): type(BINARY), data(const_cast<T*>(&x)), size(sizeof(T)) { }
			Index(const UtinyN& x): type(U_TINY_N), data(const_cast<UtinyN*>(&x)) { }
			Index(const TinyN& x): type(TINY_N), data(const_cast<TinyN*>(&x)) { }
			Index(const UshortN& x): type(U_SHORT_N), data(const_cast<UshortN*>(&x)) { }
			Index(const ShortN& x): type(SHORT_N), data(const_cast<ShortN*>(&x)) { }
			Index(const UintN& x): type(U_INT_N), data(const_cast<UintN*>(&x)) { }
			Index(const IntN& x): type(INT_N), data(const_cast<IntN*>(&x)) { }
			Index(const UbigintN& x): type(U_BIGINT_N), data(const_cast<UbigintN*>(&x)) { }
			Index(const BigintN& x): type(BIGINT_N), data(const_cast<BigintN*>(&x)) { }
			Index(const FloatN& x): type(FLOAT_N), data(const_cast<FloatN*>(&x)) { }
			Index(const DoubleN& x): type(DOUBLE_N), data(const_cast<DoubleN*>(&x)) { }
			Index(const TimeN& x): type(TIME_N), data(const_cast<TimeN*>(&x)) { }
			Index(const DateN& x): type(DATE_N), data(const_cast<DateN*>(&x)) { }
			Index(const DatetimeN& x): type(DATETIME_N), data(const_cast<DatetimeN*>(&x)) { }
			Index(const BlobN& x): type(BLOB_N), data(const_cast<BlobN*>(&x)) { }
			Index(const TextN& x): type(TEXT_N), data(const_cast<TextN*>(&x)) { }
			Index(const WtextN& x): type(WTEXT_N), data(const_cast<WtextN*>(&x)) { }
			template<int size_> Index(const NullableArray<char, size_>& x): type(CHAR_N), data(const_cast<NullableArray<char, size_>*>(&x)), size(size_) { }
			template<class T> explicit Index(const Nullable<T>& x): type(BINARY_N), data(const_cast<Nullable<T>*>(&x)), size(sizeof(T)) { }

			Index(const Index& x): type(x.type), data(x.data), size(x.size) {}
			Index(): type(NOTHING), data(0), size(0) {}

			const Index& operator=(const Index& x) { type=x.type; data=x.data; size=x.size; return *this; }
			bool operator==(const Index& x) { return type==x.type && data==x.data && size==x.size; }
		};

		/** 
		 * @brief Base data set class for communicating parameters and results with SQL queries.
		 *
		 * By deriving from this class any data structure can gain the capability to be binded to
		 * the parameters or results of an SQL query. This is accomplished polymorphically through two
		 * virtual member functions that allow the object to be treated as a container and it's member
		 * data indexed as it's elements. An example derivation follows:
@code
struct TestSet: public ASql::Data::Set
{
	size_t numberOfSqlElements() const { return 7; }
	ASql::Data::Index getInternalSqlIndex(size_t index) const
	{
		switch(index)
		{
			case 0:
				return fraction;
			case 1:
				return aDate;
			case 2:
				return aTime;
			case 3:
				return timeStamp;
			case 4:
				return someText;
			case 5:
				return someData;
			case 6:
				return ASql::Data::Index(fixedChunk, sizeof(fixedString));
			default:
				return ASql::Data::Index();
		}
	}

	ASql::Data::DoubleN fraction;
	ASql::Data::DateN aDate;
	ASql::Data::Time aTime;
	ASql::Data::DatetimeN timestamp;
	ASql::Data::WtextN someText;
	ASql::Data::BlobN someData;
	char fixedString[16];
};
@endcode
		 * Note that the indexing order must match the result column/parameter order of the
		 * SQL query.
		 *
		 * In order to be binded to a particular SQL type indexed elements in the class should be of a
		 * type that is typedefed in ASql::Data (don't worry, they are all standard types).
		 * This however is not a requirement as any plain old data structure can be indexed but it will
		 * be stored as a binary data array. Be sure to examine the constructors for Index. For a default
		 * it is best to return a default constructed Index object.
		 *
		 * @sa ASql::Data::Nullable
		 */
		struct Set
		{
			/** 
			 * @brief Get total number of indexable data members.
			 * 
			 * @return Total number of indexable data members.
			 */
			virtual size_t numberOfSqlElements() const =0;

			/** 
			 * @brief Get constant void pointer to member data.
			 *
			 * Because of the implicit constructors in Index, for most types it suffices to just return the
			 * member object itself. 
			 * 
			 * @param[in] index index Index number for member, starting at 0.
			 * 
			 * @return Constant void pointer to member data.
			 */
			virtual Index getSqlIndex(const size_t index) const =0; 

			virtual ~Set() {}
		};

		/** 
		 * @brief Creates a Set derivation.
		 *
		 * If you do now wish to burden you class definitions with vtables you can use this template to build
		 * a Set derivation from a class that isn't derived from Set yet has numberOfSqlElement() and getSqlIndex()
		 * defined.
		 *
		 * @tparam T Class type to build upon. Must define numberOfSqlElement() and getSqlIndex() described in Set
		 * but does not need to be derived from Set.
		 */
		template<class T> struct SetBuilder: public Set
		{
			T data;
			virtual size_t numberOfSqlElements() const { return data.numberOfSqlElements(); }
			virtual Index getSqlIndex(const size_t index) const { return data.getSqlIndex(index); }
		};

		/** 
		 * @brief Creates a Set derivation based on a reference.
		 *
		 * If you do now wish to burden you class definitions with vtables you can use this template to build
		 * a Set derivation from a reference to a object that isn't derived from Set yet has numberOfSqlElement()
		 * and getSqlIndex() defined.
		 *
		 * @tparam T Class type to build upon. Must define numberOfSqlElement() and getSqlIndex() described in Set
		 * but does not need to be derived from Set.
		 */
		template<class T> class SetRefBuilder: public Set
		{
			virtual size_t numberOfSqlElements() const { return m_data.numberOfSqlElements(); }
			virtual Index getSqlIndex(const size_t index) const { return m_data.getSqlIndex(index); }
			const T& m_data;
		public:
			inline SetRefBuilder(const T& x): m_data(x) {}
		};

		/** 
		 * @brief Creates a Set derivation based on a pointer.
		 *
		 * If you do now wish to burden you class definitions with vtables you can use this template to build
		 * a Set derivation from a pointer to a object that isn't derived from Set yet has numberOfSqlElement()
		 * and getSqlIndex() defined. One would use this instead of SetRefBuilder in a scenario that requires
		 * the pointed object to be changed.
		 *
		 * @tparam T Class type to build upon. Must define numberOfSqlElement() and getSqlIndex() described in Set
		 * but does not need to be derived from Set.
		 */
		template<class T> class SetPtrBuilder: public Set
		{
			const T* m_data;
			virtual size_t numberOfSqlElements() const { return m_data->numberOfSqlElements(); }
			virtual Index getSqlIndex(const size_t index) const { return m_data->getSqlIndex(index); }
		public:
			inline SetPtrBuilder(): m_data(0) {}
			inline SetPtrBuilder(const T& x): m_data(&x) {}
			inline SetPtrBuilder(SetPtrBuilder& x): m_data(x.m_data) {}
			//! Set the internal pointer
			/**
			 * @param[in] data Object to point to.
			 */
			inline void set(const T& data) { m_data=&data; }
			//! Clear the internal pointer
			inline void clear() { m_data=0; }
			//! Returns false if the internal pointer is cleared or null
			operator bool() const { return m_data; }
		};

		/** 
		 * @brief Creates a Set derivation based on a shared pointer.
		 *
		 * If you do now wish to burden you class definitions with vtables you can use this template to build
		 * a Set derivation from a shared pointer to a object that isn't derived from Set yet has numberOfSqlElement()
		 * and getSqlIndex() defined. Among other things, one would use this instead of SetPtrBuilding in say
		 * multi-threaded applications where scope is uncertain.
		 *
		 * @tparam T Class type to build upon. Must define numberOfSqlElement() and getSqlIndex() described in Set
		 * but does not need to be derived from Set.
		 */
		template<class T> class SetSharedPtrBuilder: public Set
		{
		public:
			virtual size_t numberOfSqlElements() const { return data->numberOfSqlElements(); }
			virtual Index getSqlIndex(const size_t index) const { return data->getSqlIndex(index); }
			inline SetSharedPtrBuilder() {}
			inline SetSharedPtrBuilder(const boost::shared_ptr<T>& x): data(x) {}
			inline SetSharedPtrBuilder(SetSharedPtrBuilder& x): data(x.data) {}
			boost::shared_ptr<T> data;
		};

		//! Adaptor for containers of Set objects.
		struct SetContainer
		{
			virtual Set& manufacture() =0;
			virtual void trim() =0;
			virtual ~SetContainer() {}
			virtual const Set* pull() const =0;
		};

		/** 
		 * @brief SetContainer wrapper class for STL containers.
		 *
		 * This class defines a basic container for types that have Set::getSqlIndex()
		 * and Set::numberOfSqlElements() defined yet need not be derived from Set.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions:
		 *	push_back(), back(), pop_back().
		 *
		 *	@tparam T Container type. Must be sequential.
		 */
		template<class T> class STLSetContainer: public SetContainer
		{
			mutable SetPtrBuilder<typename T::value_type> m_buffer;
			mutable typename T::iterator m_itBuffer;

			Set& manufacture()
			{
				data.push_back(typename T::value_type());
				m_buffer.set(data.back());
				return m_buffer;
			}
			void trim() { data.pop_back(); }
			const Set* pull() const
			{
				m_buffer.set(*m_itBuffer++);
				return &m_buffer;
			}
		public:
			T data;
			STLSetContainer(): m_itBuffer(data.begin()) {}
		};
	
		/** 
		 * @brief SetContainer wrapper class for referenced STL containers.
		 *
		 * This class defines a basic container for types that have Set::getSqlIndex()
		 * and Set::numberOfSqlElements() defined yet need not be derived from Set.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions:
		 *	push_back(), back(), pop_back(). One would use this instead of STLSetContainer
		 *	when the container already exists as a separate object.
		 *
		 *	@tparam T Container type. Must be sequential.
		 */
		template<class T> class STLSetRefContainer: public SetContainer
		{
			T& data;
			mutable SetPtrBuilder<typename T::value_type> m_buffer;
			mutable typename T::iterator m_itBuffer;

			Set& manufacture()
			{
				data.push_back(typename T::value_type());
				m_buffer.set(data.back());
				return m_buffer;
			}
			void trim() { data.pop_back(); }
			const Set* pull() const
			{
				m_buffer.set(*m_itBuffer++);
				return &m_buffer;
			}
		public:
			STLSetRefContainer(T& x): data(x), m_itBuffer(data.begin()) {}
		};
	
		/** 
		 * @brief SetContainer wrapper class for shared STL containers.
		 *
		 * This class defines a basic container for types that have Set::getSqlIndex()
		 * and Set::numberOfSqlElements() defined yet need not be derived from Set.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions:
		 *	push_back(), back(), pop_back(). One would use this instead of STLRefContainer
		 *	when the container object needs consistent scope. Say for multi threading applications.
		 *
		 *	@tparam T Container type. Must be sequential.
		 */
		template<class T> class STLSharedSetContainer: public SetContainer
		{
			mutable SetPtrBuilder<typename T::value_type> m_buffer;
			mutable typename T::iterator m_itBuffer;

			Set& manufacture()
			{
				data->push_back(typename T::value_type());
				m_buffer.set(data->back());
				return m_buffer;
			}
			void trim() { data->pop_back(); }
			const Set* pull() const
			{
				m_buffer.set(*m_itBuffer++);
				return &m_buffer;
			}
		public:
			boost::shared_ptr<T> data;
			STLSharedSetContainer(const boost::shared_ptr<T>& x): data(x) {}
			STLSharedSetContainer(): m_itBuffer(data->begin()) {}
		};

		/** 
		 * @brief Handle data conversion from standard data types to internal SQL engine types.
		 */
		struct Conversion
		{
			/** 
			 * @brief Get a pointer to the internal data.
			 * 
			 * @return Void pointer to internal data.
			 */
			virtual void* getPointer() =0;

			/** 
			 * @brief Convert SQL query results.
			 */
			virtual void convertResult() =0;

			/** 
			 * @brief Convert SQL query parameters.
			 */
			virtual void convertParam() =0;

			/** 
			 * @brief Pointer to standard data type.
			 */
			void* external;
		};
		
		typedef std::map<int, boost::shared_ptr<Conversion> > Conversions;
	}

	//! Stores all query results and parameters
	/**
	 * This class is intended as a thread safe mechanism of storing parameter and results data for a queued
	 * SQL query. It can also be used to cancel queries. The query object should have destruction control over
	 * the result set and parameter set to prevent thread related seg-faults.
	 *
	 * The original query object (not created from a copy constructor) can and by default will cancel the query
	 * when it goes out of scope.
	 */
	class Query
	{
	private:
		struct SharedData
		{
			enum Flags { FLAG_SINGLE_PARAMETERS=1, FLAG_SINGLE_RESULTS=1<<1, FLAG_NO_MANAGE_RESULTS=1<<2, FLAG_NO_MANAGE_PARAMETERS=1<<3 };
			SharedData(): m_parameters(0), m_results(0), m_insertId(0), m_rows(0), m_cancel(false), m_flags(FLAG_SINGLE_PARAMETERS) {}
			~SharedData()
			{
				delete m_rows;
				delete m_insertId;
				destroyResults();
				destroyParameters();
			}
			void* m_parameters;
			void* m_results;
			unsigned long long int* m_insertId;
			unsigned long long int* m_rows;
			Error m_error;
			boost::function<void()> m_callback;
			boost::mutex m_callbackMutex;
			bool m_cancel;
			unsigned char m_flags;

			void destroyResults()
			{
				if(m_flags&FLAG_NO_MANAGE_RESULTS) return;
				if(m_flags&FLAG_SINGLE_RESULTS)
					delete static_cast<Data::Set*>(m_results);
				else
					delete static_cast<Data::SetContainer*>(m_results);
			}

			void destroyParameters()
			{
				if(m_flags&FLAG_NO_MANAGE_PARAMETERS) return;
				if(m_flags&FLAG_SINGLE_PARAMETERS)
					delete static_cast<Data::Set*>(m_parameters);
				else
					delete static_cast<Data::SetContainer*>(m_parameters);
			}
		};

		boost::shared_ptr<SharedData> m_sharedData;
		unsigned char m_flags;

		enum Flags { FLAG_ORIGINAL=1, FLAG_KEEPALIVE=1<<1 };

		void callback()
		{
			boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex);
			if(!m_sharedData->m_callback.empty())
				m_sharedData->m_callback();
		}

		template<class T> friend class ConnectionPar;

	public:
		Query(): m_sharedData(new SharedData), m_flags(FLAG_ORIGINAL) { }
		Query(const Query& x): m_sharedData(x.m_sharedData), m_flags(0) {}
		~Query()
		{
			if(m_flags == FLAG_ORIGINAL)
				cancel(true);
		}
		//! Retrieve the auto incremented insert ID of the query.
		unsigned int insertId() const { return m_sharedData->m_insertId?*(m_sharedData->m_insertId):0; }
		//! Retrieve the total row count of the query.
		unsigned int rows() const { return m_sharedData->m_rows?*(m_sharedData->m_rows):0; }
		//! Test if the query is still being processed.
		/**
		 * @return True if the query is still working.
		 */
		bool busy() const { return m_sharedData.use_count() != 1; }
		//! Retrieve the Error object associated with the query.
		/**
		 * If no error occurred then the erno of the object is 0.
		 */
		Error error() const { return m_sharedData->m_error; }

		//! Set a function to be called when the query is finished.
		void setCallback(boost::function<void()> callback=boost::function<void()>())
		{
			boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex);
			m_sharedData->m_callback = callback;
		}
		//! If set true, the query will not be cancelled when the original object goes out of scope.
		void keepAlive(bool x) { if(x) m_flags|=FLAG_KEEPALIVE; else m_flags&=~FLAG_KEEPALIVE; }
		//! Tell the query to cancel operation.
		void cancel(bool x) { m_sharedData->m_cancel = x; }
		//! Enabled fetching of total rows.
		void enableRows(bool x)
		{
			if(x)
			{
				if(!m_sharedData->m_rows)
					m_sharedData->m_rows = new unsigned long long;
			}
			else
			{
				delete m_sharedData->m_rows;
				m_sharedData->m_rows = 0;
			}
		}
		//! Enabled fetching of last auto increment insert ID
		void enableInsertId(bool x)
		{
			if(x)
			{
				if(!m_sharedData->m_insertId)
					m_sharedData->m_insertId = new unsigned long long;
			}
			else
			{
				delete m_sharedData->m_insertId;
				m_sharedData->m_insertId = 0;
			}
		}

		//! Set a single row result set.
		/**
		 * This is used for single row result sets. The passed object should be dynamically allocated and
		 * the query object should control it's deletion.
		 *
		 * The strength of this class lies in the fact that if it goes out of scope, the associated data is kept around until
		 * the query actually finishes.
		 *
		 * If you do not want the query object to handle the deletion of the object you must call manageResults()
		 */
		void setResults(Data::Set* results) { m_sharedData->destroyResults(); m_sharedData->m_results=results; m_sharedData->m_flags|=SharedData::FLAG_SINGLE_RESULTS; }
		//! Set a multiple row result set.
		/**
		 * This is used for single row result sets. The passed object should be dynamically allocated and
		 * the query object should control it's deletion.
		 *
		 * If you do not want the query object to handle the deletion of the object you must call manageResults()
		 */
		void setResults(Data::SetContainer* results) { m_sharedData->destroyResults(); m_sharedData->m_results=results; m_sharedData->m_flags&=~SharedData::FLAG_SINGLE_RESULTS; }
		//! Set a single row parameter set.
		/**
		 * This is used for single row parameter sets. The passed object should be dynamically allocated and
		 * the query object should control it's deletion.
		 *
		 * If you do not want the query object to handle the deletion of the object you must call manageParameters()
		 */
		void setParameters(Data::Set* parameters) { m_sharedData->destroyParameters(); m_sharedData->m_parameters=parameters; m_sharedData->m_flags|=SharedData::FLAG_SINGLE_PARAMETERS; }
		//! Set a multi-row parameter set.
		/**
		 * This is used for multiple row parameter sets. The passed object should be dynamically allocated and
		 * the query object should control it's deletion.
		 *
		 * If you do not want the query object to handle the deletion of the object you must call manageParameters()
		 */
		void setParameters(Data::SetContainer* parameters) { m_sharedData->destroyParameters(); m_sharedData->m_parameters=parameters; m_sharedData->m_flags &= ~SharedData::FLAG_SINGLE_PARAMETERS; }
		//! Pass false if you do not want Query to handle deallocation of the results.
		void manageResults(bool manage) { if(manage) m_sharedData->m_flags&=~SharedData::FLAG_NO_MANAGE_RESULTS; else m_sharedData->m_flags|=SharedData::FLAG_NO_MANAGE_RESULTS; }
		//! Pass false if you do not want Query to handle deallocation of the parameters.
		void manageParameters(bool manage) { if(manage) m_sharedData->m_flags&=~SharedData::FLAG_NO_MANAGE_PARAMETERS; else m_sharedData->m_flags|=SharedData::FLAG_NO_MANAGE_PARAMETERS; }

		//! Quick helper function to ease the creation of result sets.
		/**
		 * This function simply calls setResults(new T) and then returns a statically casted pointer
		 * to them.
		 */
		template<class T> T* createResults() { setResults(new T); return static_cast<T*>(results()); }
		//! Quick helper function to ease the creation of parameter sets.
		/**
		 * This function simply calls setParameters(new T) and then returns a statically casted pointer
		 * to them.
		 */
		template<class T> T* createParameters() { setParameters(new T); return static_cast<T*>(parameters()); }

		//! Release the result set from the query objects control but don't actually deallocate it.
		void relinquishResults() { m_sharedData->m_results=0; m_sharedData->m_flags &= ~SharedData::FLAG_SINGLE_RESULTS; }
		//! Release the parameter set from the query objects control but don't actually deallocate it.
		void relinquishParameters() { m_sharedData->m_parameters=0; m_sharedData->m_flags |= SharedData::FLAG_SINGLE_PARAMETERS; }

		//! Get a pointer to the result set.
		/**
		 * This will return a void* pointer to the result set. You need to cast it to whatever it actually is.
		 * Be extra careful when dealing with multiple inheritance scenarios. Always cast the pointer to either a
		 * Data::SetContainer or Data::Set (depending on what it is) before casting it to your type.
		 */
		void* results() { return m_sharedData->m_results; }
		//! Get a pointer to the parameter set.
		/**
		 * This will return a void* pointer to the parameter set. You need to cast it to whatever it actually is.
		 * Be extra careful when dealing with multiple inheritance scenarios. Always cast the pointer to either a
		 * Data::SetContainer or Data::Set (depending on what it is) before casting it to your type.
		 */
		void* parameters() { return m_sharedData->m_parameters; }

		//! Deallocate the result set.
		void clearResults() { m_sharedData->destroyResults(); relinquishResults(); }
		//! Deallocate the parameter set.
		void clearParameters() { m_sharedData->destroyParameters(); relinquishParameters(); }

		//! Reset the query object to new state. For recycling objects without re-construction.
		void reset() { m_sharedData.reset(new SharedData); cancel(false); keepAlive(false); }

		/*void swap()
		{
			unsigned char flags(m_sharedData->m_flags);
			void* results(m_sharedData->m_results);
			m_sharedData->m_results = m_sharedData->m_parameters;
			m_sharedData->m_parameters = results;
		}*/
	};

	/** 
	 * @brief %SQL %Connection.
	 */
	class Connection
	{
	protected:
		/** 
		 * @brief Number of threads to pool for simultaneous queries.
		 */
		const int maxThreads;
		boost::mutex threadsMutex;
		boost::condition_variable threadsChanged;
		int threads;

		boost::condition_variable wakeUp;

		boost::mutex terminateMutex;
		bool terminateBool;

		Connection(const int maxThreads_): maxThreads(maxThreads_), threads(0) {}
	};

	template<class T> class ConnectionPar: private Connection
	{
	private:
		struct QuerySet
		{
			QuerySet(Query& query, T* const& statement): m_query(query), m_statement(statement) {}
			Query m_query;
			T* const m_statement;
		};
		/** 
		 * @brief Thread safe queue of queries.
		 */
		class Queries: public std::queue<QuerySet>, public boost::mutex {} queries;

		/** 
		 * @brief Function that runs in threads.
		 */
		void intHandler();

		class TakeStatement
		{
			boost::lock_guard<boost::mutex> m_lock;
			const bool*& m_canceler;
		public:
			TakeStatement(const bool*& canceler, bool& dest, boost::mutex& mutex): m_lock(mutex), m_canceler(canceler) { canceler=&dest; }
			~TakeStatement() { m_canceler=s_false; }			
		};

	protected:
		ConnectionPar(const int maxThreads_): Connection(maxThreads_) {}
		inline void queue(T* const& statement, Query& query);
	public:
		//! Start the query handling threads.
		void start();
		//! Terminator the query handling threads.
		void terminate();

		static const bool s_false;
	};	

	/** 
	 * @brief SQL %Statement.
	 */
	class Statement
	{
	protected:
		Data::Conversions paramsConversions;
		Data::Conversions resultsConversions;
	};
}

template<class T> void ASql::ConnectionPar<T>::start()
{
	{
		boost::lock_guard<boost::mutex> terminateLock(terminateMutex);
		terminateBool=false;
	}
	
	boost::unique_lock<boost::mutex> threadsLock(threadsMutex);
	while(threads<maxThreads)
	{
		boost::thread(boost::bind(&ConnectionPar<T>::intHandler, boost::ref(*this)));
		threadsChanged.wait(threadsLock);
	}
}

template<class T> void ASql::ConnectionPar<T>::terminate()
{
	{
		boost::lock_guard<boost::mutex> terminateLock(terminateMutex);
		terminateBool=true;
	}
	wakeUp.notify_all();

	boost::unique_lock<boost::mutex> threadsLock(threadsMutex);
	while(threads)
		threadsChanged.wait(threadsLock);
}

template<class T> void ASql::ConnectionPar<T>::intHandler()
{
	{
		boost::lock_guard<boost::mutex> threadsLock(threadsMutex);
		++threads;
	}
	threadsChanged.notify_one();
	
	boost::unique_lock<boost::mutex> terminateLock(terminateMutex, boost::defer_lock_t());
	boost::unique_lock<boost::mutex> queriesLock(queries, boost::defer_lock_t());

	while(1)
	{
		terminateLock.lock();
		if(terminateBool)
			break;
		terminateLock.unlock();

		queriesLock.lock();
		if(!queries.size())
		{
			wakeUp.wait(queriesLock);
			queriesLock.unlock();
			continue;
		}

		QuerySet querySet(queries.front());
		queries.pop();
		queriesLock.unlock();

		Error error;

		querySet.m_statement->m_stop = &(querySet.m_query.m_sharedData->m_cancel);
		try
		{
			TakeStatement takeStatement(querySet.m_statement->m_stop, querySet.m_query.m_sharedData->m_cancel, querySet.m_statement->executeMutex);
			if(querySet.m_query.m_sharedData->m_flags & Query::SharedData::FLAG_SINGLE_PARAMETERS)
			{
				if(querySet.m_query.m_sharedData->m_flags & Query::SharedData::FLAG_SINGLE_RESULTS)
				{
					if(!querySet.m_statement->execute(static_cast<const Data::Set*>(querySet.m_query.parameters()), *static_cast<Data::Set*>(querySet.m_query.results()))) querySet.m_query.clearResults();
				}
				else
					querySet.m_statement->execute(static_cast<const Data::Set*>(querySet.m_query.parameters()), static_cast<Data::SetContainer*>(querySet.m_query.results()), querySet.m_query.m_sharedData->m_insertId, querySet.m_query.m_sharedData->m_rows);
			}
			else
			{
				querySet.m_statement->execute(*static_cast<const Data::SetContainer*>(querySet.m_query.parameters()), querySet.m_query.m_sharedData->m_rows);
			}
		}
		catch(const Error& e)
		{
			querySet.m_query.m_sharedData->m_error=e;
			std::cerr << e.msg << '\n';
		}

		querySet.m_query.callback();
	}

	{
		boost::lock_guard<boost::mutex> threadsLock(threadsMutex);
		--threads;
	}
	threadsChanged.notify_one();
}


template<class T> void ASql::ConnectionPar<T>::queue(T* const& statement, Query& query)
{
	boost::lock_guard<boost::mutex> queriesLock(queries);
	queries.push(QuerySet(query, statement));
	wakeUp.notify_one();
}

template<class T> const bool ASql::ConnectionPar<T>::s_false = false;

#endif
