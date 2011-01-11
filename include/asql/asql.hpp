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
		 * Note that in most cases you are better off using one the following templated wrapper classes 
		 * to associate you data set with ASql. This eliminates the overhead of virtual functions from
		 * your data type if it is used in a computationally intensive situation.
		 * - SetBuilder
		 * - SetRefBuilder
		 * - SetPtrBuilder
		 * - SetSharedPtrBuilder
		 *
		 * If you use this technique you MUST still define the numberOfSqlElements() and getSqlIndex() in
		 * your dataset as below but do not derive from Set. The function will be called from the templates
		 * instead of virtually.
		 *
		 * By deriving from this class any data structure can gain the capability to be binded to
		 * the parameters or results of an SQL query. This is accomplished polymorphically through two
		 * virtual member functions that allow the object to be treated as a container and it's member
		 * data indexed as it's elements. An example derivation follows:
@code
struct TestSet: public ASql::Data::Set
{
	size_t numberOfSqlElements() const { return 7; }
	ASql::Data::Index getSqlIndex(size_t index) const
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
		 * @brief Wraps a Set object around an new auto-allocated dataset of type T
		 *
		 * @tparam T object type to create. Must have %numberOfSqlElements() and  %getSqlIndex() function defined as
		 * per the instruction in Data::Set.
		 */
		template<class T> class SetBuilder: public Set
		{
		public:
			/** 
			 * @brief Dataset object of type T that the Set is wrapped around. This is your object.
			 */
			T data;
		private:
			/** 
			 * @brief Wrapper function for the %numberOfSqlElements() function in the data object.
			 */
			virtual size_t numberOfSqlElements() const { return data.numberOfSqlElements(); }
			/** 
			 * @brief Wrapper function for the %getSqlIndex() function in the data object.
			 */
			virtual Index getSqlIndex(const size_t index) const { return data.getSqlIndex(index); }
		};

		/** 
		 * @brief Wraps a Set object around a reference to a dataset of type T
		 *
		 * @tparam T object type to reference to. Must have %numberOfSqlElements() and  %getSqlIndex() function defined as
		 * per the instruction in Data::Set.
		 */
		template<class T> class SetRefBuilder: public Set
		{
			/** 
			 * @brief Wrapper function for the %numberOfSqlElements() function in the data object.
			 */
			virtual size_t numberOfSqlElements() const { return m_data.numberOfSqlElements(); }
			/** 
			 * @brief Wrapper function for the %getSqlIndex() function in the data object.
			 */
			virtual Index getSqlIndex(const size_t index) const { return m_data.getSqlIndex(index); }
			/** 
			 * @brief Reference to the dataset
			 */
			const T& m_data;
		public:
			/** 
			 * @param x Reference to the dataset of type T to reference to.
			 */
			inline SetRefBuilder(const T& x): m_data(x) {}
		};

		/** 
		 * @brief Wraps a Set object around a pointer to a dataset of type T
		 *
		 * This has the one advantage over SetRefBuilder in that the dataset pointed to can be changed with destroying/rebuilding
		 * wrapper object.
		 *
		 * @tparam T object type to point to. Must have %numberOfSqlElements() and  %getSqlIndex() function defined as
		 * per the instruction in Data::Set.
		 */
		template<class T> class SetPtrBuilder: public Set
		{
			/** 
			 * @brief Pointer to the dataset
			 */
			const T* m_data;
			/** 
			 * @brief Wrapper function for the %numberOfSqlElements() function in the data object.
			 */
			virtual size_t numberOfSqlElements() const { return m_data->numberOfSqlElements(); }
			/** 
			 * @brief Wrapper function for the %getSqlIndex() function in the data object.
			 */
			virtual Index getSqlIndex(const size_t index) const { return m_data->getSqlIndex(index); }
		public:
			/** 
			 * @brief Default constructor set's the pointer to null
			 */
			inline SetPtrBuilder(): m_data(0) {}
			/** 
			 * @brief Set the pointer to the address of the object referenced to by x
			 */
			inline SetPtrBuilder(const T& x): m_data(&x) {}
			inline SetPtrBuilder(SetPtrBuilder& x): m_data(x.m_data) {}
			/** 
			 * @brief Set the pointer to the address of the object referenced to by x
			 */
			inline void set(const T& data) { m_data=&data; }
			/** 
			 * @brief Set the pointer to null
			 */
			inline void clear() { m_data=0; }
			/** 
			 * @brief Return true if the pointer is not null
			 */
			operator bool() const { return m_data; }
		};

		/** 
		 * @brief Wraps a Set object around a shared pointer to a dataset of type T
		 *
		 * This has the one advantage over SetRefBuilder in that the dataset pointed to can be changed with destroying/rebuilding
		 * wrapper object and SetPtrBuilder in that the pointer can be shared.
		 *
		 * @tparam T object type to point to. Must have %numberOfSqlElements() and  %getSqlIndex() function defined as
		 * per the instruction in Data::Set.
		 */
		template<class T> class SetSharedPtrBuilder: public Set
		{
			/** 
			 * @brief Wrapper function for the %numberOfSqlElements() function in the data object.
			 */
			virtual size_t numberOfSqlElements() const { return data->numberOfSqlElements(); }
			/** 
			 * @brief Wrapper function for the %getSqlIndex() function in the data object.
			 */
		public:
			virtual Index getSqlIndex(const size_t index) const { return data->getSqlIndex(index); }
			inline SetSharedPtrBuilder() {}
			inline SetSharedPtrBuilder(const boost::shared_ptr<T>& x): data(x) {}
			inline SetSharedPtrBuilder(SetSharedPtrBuilder& x): data(x.data) {}
			/** 
			 * @brief Shared pointer to the dataset
			 */
			boost::shared_ptr<T> data;
		};

		/** 
		 * @brief Wraps a Set object around an new auto-allocated individual object of type T
		 *
		 * @tparam T object type to create.
		 */
		template<class T> class IndySetBuilder: public Set
		{
		public:
			/** 
			 * @brief Object of type T that the Set is wrapped around. This is your object.
			 */
			T data;
		private:
			/** 
			 * @brief Returns 1 as it is an individual container
			 */
			virtual size_t numberOfSqlElements() const { return 1; }
			/** 
			 * @brief Just returns an index to data.
			 */
			virtual Index getSqlIndex(const size_t index) const { return data; }
		};

		/** 
		 * @brief Wraps a Set object around a reference to an individual object of type T
		 *
		 * @tparam T object type to create.
		 */
		template<class T> class IndySetRefBuilder: public Set
		{
			/** 
			 * @brief Object of type T that the Set is wrapped around. This is your object.
			 */
			const T& data;
			/** 
			 * @brief Returns 1 as it is an individual container
			 */
			virtual size_t numberOfSqlElements() const { return 1; }
			/** 
			 * @brief Just returns an index to data.
			 */
			virtual Index getSqlIndex(const size_t index) const { return data; }
		public:
			/** 
			 * @param x Reference to the object of type T to reference to.
			 */
			inline IndySetRefBuilder(const T& x): data(x) {}
		};

		/** 
		 * @brief Base class for containers of Data::Set objects to be used for result/parameter data in SQL queries.
		 */
		struct SetContainer
		{
			/** 
			 * @brief Appends a row to the container and returns a reference to it.
			 */
			virtual Set& manufacture() =0;
			/** 
			 * @brief Pop a row off the end of the container.
			 */
			virtual void trim() =0;
			virtual ~SetContainer() {}
			/** 
			 * @brief Get a row from the front and move on to the next row.
			 * 
			 * @return This function should return a pointer to the row or null if at the end.
			 */
			virtual const Set* pull() const =0;
		};

		/** 
		 * @brief Wraps a SetContainer object around a new auto-allocated STL container of type T
		 *
		 * This class defines a basic container for types that can be wrapped by the Set class.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions
		 *	push_back(), back(), pop_back() and it's content type must be wrappable by Set as per the
		 *	instructions there.
		 *
		 *	@tparam Container type. Must be sequential.
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
			 /** 
			 * @brief STL container object of type T that the SetContainer is wrapped around. This is your object.
			 */
			T data;
			STLSetContainer(): m_itBuffer(data.begin()) {}
		};
	
		/** 
		 * @brief Wraps a SetContainer object around a reference to an STL container of type T
		 *
		 * This class defines a basic container for types that can be wrapped by the Set class.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions
		 *	push_back(), back(), pop_back() and it's content type must be wrappable by Set as per the
		 *	instructions there.
		 *
		 *	@tparam Container type. Must be sequential.
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
			/** 
			 * @param x Reference to the STL container of type T to reference to.
			 */
			STLSetRefContainer(T& x): data(x), m_itBuffer(data.begin()) {}
		};
	
		/** 
		 * @brief Wraps a SetContainer object around a shared pointer to an STL container of type T
		 *
		 * This class defines a basic container for types that can be wrapped by the Set class.
		 *	It is intended for retrieving multi-row results from SQL queries. In order to
		 *	function the passed container type must have the following member functions
		 *	push_back(), back(), pop_back() and it's content type must be wrappable by Set as per the
		 *	instructions there.
		 *
		 *	@tparam Container type. Must be sequential.
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
			/** 
			 * @brief Shared pointer to the STL container
			 */
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

	/** 
	 * @brief Class for storing query data to be passed and retrieved from statements.
	 * 
	 * This will contain all data associated with an individual query. With it one can retrieve results, set arguments,
	 * and set various query parameters that can be checked/set with the public access functions.
	 *
	 * Most of the data is stored in a shared data structure that is referenced by copies of a Query object. The only non-shared
	 * data stored in the class is the m_flags object which set's two possible flags. If the query is built from the default constructor,
	 * the FLAG_ORIGINAL flag is set. This serves the purpose of keeping track of which query object actually owns the original data. With
	 * this, one can have the query cancelled in it's handler thread if the original query object goes out of scope in it's own thread.
	 * If one wishes to avoid this behaviour they can have the FLAG_KEEPALIVE flag set with the keepAlive() function.
	 */
	class Query
	{
	private:
		/** 
		 * @brief Sub-structure to store shared data for the query
		 */
		struct SharedData
		{
			/** 
			 * @brief Contains the possible flags a query can have set.
			 */
			/** 
			 * @brief Only constructor
			 *
			 * The default constructor zeros all data and sets the shared flags to FLAG_SINGLE_PARAMETERS.
			 */
			enum Flags { FLAG_SINGLE_PARAMETERS=1, FLAG_SINGLE_RESULTS=1<<1, FLAG_NO_MANAGE_RESULTS=1<<2, FLAG_NO_MANAGE_PARAMETERS=1<<3 };
			SharedData(): m_parameters(0), m_results(0), m_insertId(0), m_rows(0), m_cancel(false), m_flags(FLAG_SINGLE_PARAMETERS) {}
			/** 
			 * @brief Destroys the shared data.
			 *
			 * This should obviously only get called when at last all associated Query objects go out of scope.
			 */
			~SharedData()
			{
				delete m_rows;
				delete m_insertId;
				destroyResults();
				destroyParameters();
			}
			/** 
			 * @brief Pointer to the data object containing the parameters to be passed to the statement.
			 *
			 * This is a void pointer so that it may contain a single row of parameters (Data::Set) or multiple rows of parameters
			 * (Data::SetContainer). Multiple rows of parameters is in effect just executing the query multiple times in a single
			 * transaction.
			 */
			void* m_parameters;
			/** 
			 * @brief Pointer to the data object that should store the results returned from the statement.
			 *
			 * This is a void pointer so that it may contain a single row of results (Data::Set) or multiple rows of results
			 * (Data::SetContainer).
			 */
			void* m_results;
			/** 
			 * @brief Used to return a potential last auto increment insert id.
			 */
			unsigned long long int* m_insertId;
			/** 
			 * @brief Used to return the number of rows available/affected by the query.
			 */
			unsigned long long int* m_rows;
			/** 
			 * @brief Contains the error (if any) returned by the query.
			 */
			Error m_error;
			/** 
			 * @brief A callback function to be called when the query completes.
			 */
			boost::function<void()> m_callback;
			boost::mutex m_callbackMutex;
			/** 
			 * @brief If set true the query should cancel when the opportunity arises.
			 */
			bool m_cancel;
			/** 
			 * @brief flags for the shared data
			 *
			 * FLAG_NO_MANAGE_RESULTS: This means that result data should not be freed when
			 * the shared data structure goes out of scope. One would use this if that are done
			 * with the query object but want control over the scope of the results.
			 *
			 * FLAG_SINGLE_RESULTS: This means that the data pointed to be m_results is a Data::Set
			 * and not a Data::SetContainer.
			 *
			 * FLAG_NO_MANAGE_PARAMETERS: This means that parameter data should not be freed when
			 * the shared data structure goes out of scope. One would use this if that are done
			 * with the query object but want control over the scope of the parameters.
			 *
			 * FLAG_SINGLE_RESULTS: This means that the data pointed to be m_parameters is a Data::Set
			 * and not a Data::SetContainer.
			 */
			unsigned char m_flags;

			/** 
			 * @brief Safely destroys(or doesn't) the result data set.
			 *
			 * This will destroy the result set if FLAG_NO_MANAGE_RESULTS is not set and set the
			 * m_results pointer to null.
			 */
			void destroyResults()
			{
				if(m_flags&FLAG_NO_MANAGE_RESULTS) return;
				if(m_flags&FLAG_SINGLE_RESULTS)
					delete static_cast<Data::Set*>(m_results);
				else
					delete static_cast<Data::SetContainer*>(m_results);
			}

			/** 
			 * @brief Safely destroys(or doesn't) the parameter data set.
			 *
			 * This will destroy the parameter set if FLAG_NO_MANAGE_PARAMETERS is not set and set the
			 * m_parameters pointer to null.
			 */
			void destroyParameters()
			{
				if(m_flags&FLAG_NO_MANAGE_PARAMETERS) return;
				if(m_flags&FLAG_SINGLE_PARAMETERS)
					delete static_cast<Data::Set*>(m_parameters);
				else
					delete static_cast<Data::SetContainer*>(m_parameters);
			}
		};

		/** 
		 * @brief Shared pointer to query data.
		 */
		boost::shared_ptr<SharedData> m_sharedData;
		/** 
		 * @brief Flags for individual query object.
		 * 
		 * FLAG_ORIGINAL: Set only if this query object was constructed with the default constructor. Never set if constructed from the
		 * copy constructor.
		 *
		 *	FLAG_KEEPALIVE: Set if the query should not be cancelled if the original object goes out of scope before the query is complete.
		 */
		unsigned char m_flags;

		enum Flags { FLAG_ORIGINAL=1, FLAG_KEEPALIVE=1<<1 };

		/** 
		 * @brief Lock and call the callback function of it exists.
		 */
		void callback()
		{
			boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex);
			if(!m_sharedData->m_callback.empty())
				m_sharedData->m_callback();
		}
		

		template<class T> friend class ConnectionPar;

	public:
		/** 
		 * @brief Calling the default constructor will set FLAG_ORIGINAL and create new shared data.
		 */
		Query(): m_sharedData(new SharedData), m_flags(FLAG_ORIGINAL) { }
		/** 
		 * @brief Calling the copy constructor will not set FLAG_ORIGINAL or create new shared data.
		 * 
		 * @param x May be a copy or original Query object.
		 */
		Query(const Query& x): m_sharedData(x.m_sharedData), m_flags(0) {}
		/** 
		 * @brief If the original object, cancel() is called.
		 */
		~Query()
		{
			if(m_flags == FLAG_ORIGINAL)
				cancel();
		}
		/** 
		 * @brief Returns the insert ID returned from the query or 0 if nothing.
		 */
		unsigned int insertId() const { return m_sharedData->m_insertId?*(m_sharedData->m_insertId):0; }
		/** 
		 * @brief Returns the rows affected/available from the query or 0 if nothing.
		 * 
		 * Note that if using for the number of rows from a query this will represent the number of rows
		 * available before a LIMIT.
		 */
		unsigned int rows() const { return m_sharedData->m_rows?*(m_sharedData->m_rows):0; }
		/** 
		 * @brief Returns true if copies of this query still exist (query is still working in another thread).
		 */
		bool busy() const { return m_sharedData.use_count() != 1; }
		/** 
		 * @brief Return the error object associated with the query.
		 *
		 * Note this will be a default Error object if there was no error.
		 */
		Error error() const { return m_sharedData->m_error; }

		/** 
		 * @brief Set the callback function to be called at the end of the query.
		 *
		 * Note that this will be called even if there is an error or the query can cancelled.
		 */
		void setCallback(boost::function<void()> callback=boost::function<void()>())
		{
			boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex);
			m_sharedData->m_callback = callback;
		}
		/** 
		 * @brief Return true if a callback is associated with this query
		 */
		bool isCallback() { return !m_sharedData->m_callback.empty(); }
		/** 
		 * @brief Get the callback function
		 */
		boost::function<void()> getCallback()
		{
			return m_sharedData->m_callback;
		}
		/** 
		 * @brief Set true if you want the query to not be cancelled when the original object is destroyed.
		 *
		 * Note that the default is to cancel and this must be called from the original query.
		 */
		void keepAlive(bool x) { if(x) m_flags|=FLAG_KEEPALIVE; else m_flags&=~FLAG_KEEPALIVE; }
		/** 
		 * @brief Call this function to cancel the query.
		 *
		 * This will cancel the query at the earliest opportunity. Calling a cancel will rollback any changes
		 * in the associated transaction.
		 */
		void cancel() { m_sharedData->m_cancel = true; }
		/** 
		 * @brief Call for some reason if you want to clear a cancel request
		 */
		void clearCancel() { m_sharedData->m_cancel = false; }
		/** 
		 * @brief Call this function to enable the retrieval of a row count (affected/available rows)
		 */
		void enableRows()
		{
			if(!m_sharedData->m_rows)
				m_sharedData->m_rows = new unsigned long long;
		}
		/** 
		 * @brief Call this function to enable the retrieval of an auto-increment insert ID
		 */
		void enableInsertId()
		{
			if(!m_sharedData->m_insertId)
				m_sharedData->m_insertId = new unsigned long long;
		}

		/** 
		 * @brief Set's the shared data to point to the passed single row result set.
		 *
		 * Note that the Query class assumes responsibility for destroying the result set unless explicitly told not to
		 * with manageResults().
		 */
		void setResults(Data::Set* results) { m_sharedData->destroyResults(); m_sharedData->m_results=results; m_sharedData->m_flags|=SharedData::FLAG_SINGLE_RESULTS; }
		/** 
		 * @brief Set's the shared data to point to the passed multi-row result set.
		 *
		 * Note that the Query class assumes responsibility for destroying the result set unless explicitly told not to
		 * with manageResults().
		 */
		void setResults(Data::SetContainer* results) { m_sharedData->destroyResults(); m_sharedData->m_results=results; m_sharedData->m_flags&=~SharedData::FLAG_SINGLE_RESULTS; }
		/** 
		 * @brief Set's the shared data to point to the passed single row parameter set.
		 *
		 * Note that the Query class assumes responsibility for destroying the parameter set unless explicitly told not to
		 * with manageParameters().
		 */
		void setParameters(Data::Set* parameters) { m_sharedData->destroyParameters(); m_sharedData->m_parameters=parameters; m_sharedData->m_flags|=SharedData::FLAG_SINGLE_PARAMETERS; }
		/** 
		 * @brief Set's the shared data to point to the passed multi-row parameter set.
		 *
		 * Note that the Query class assumes responsibility for destroying the parameter set unless explicitly told not to
		 * with manageParameters().
		 */
		void setParameters(Data::SetContainer* parameters) { m_sharedData->destroyParameters(); m_sharedData->m_parameters=parameters; m_sharedData->m_flags &= ~SharedData::FLAG_SINGLE_PARAMETERS; }
		/** 
		 * @brief Set whether or not the Query object should manage destruction of the result data.
		 *
		 * True means that it will destroy the result data when it sees fit. False leaves that responsibility to you (the memory will leak
		 * if you don't explicitly delete it at some point. This obviously defaults to true.
		 *
		 * Be very careful with this. Often times relinquishResults() will be a better option for what you might be trying to do.
		 */
		void manageResults(bool manage) { if(manage) m_sharedData->m_flags&=~SharedData::FLAG_NO_MANAGE_RESULTS; else m_sharedData->m_flags|=SharedData::FLAG_NO_MANAGE_RESULTS; }
		/** 
		 * @brief Set whether or not the Query object should manage destruction of the parameter data.
		 *
		 * True means that it will destroy the parameter data when it sees fit. False leaves that responsibility to you (the memory will leak
		 * if you don't explicitly delete it at some point. This obviously defaults to true.
		 *
		 * Be very careful with this. Often times relinquishParameters() will be a better option for what you might be trying to do.
		 */
		void manageParameters(bool manage) { if(manage) m_sharedData->m_flags&=~SharedData::FLAG_NO_MANAGE_PARAMETERS; else m_sharedData->m_flags|=SharedData::FLAG_NO_MANAGE_PARAMETERS; }

		/** 
		 * @brief Little inline helper function to create result sets of a specific type.
		 * 
		 * It will return a pointer to the newly created object and set it as the Query objects result set.
		 * 
		 * @tparam T Type to create. Must be derived from either Data::Set or Data::SetContainer. Suggest using one of the
		 * template wrapper types defined in Data.
		 */
		template<class T> T* createResults() { setResults(new T); return static_cast<T*>(results()); }
		/** 
		 * @brief Little inline helper function to create parameter sets of a specific type.
		 * 
		 * It will return a pointer to the newly created object and set it as the Query objects parameter set.
		 * 
		 * @tparam T Type to create. Must be derived from either Data::Set or Data::SetContainer. Suggest using one of the
		 * template wrapper types defined in Data.
		 */
		template<class T> T* createParameters() { setParameters(new T); return static_cast<T*>(parameters()); }

		/** 
		 * @brief Relinquishes control over the result set.
		 *
		 * Calling this will disassociate the query object (and it's copies) from the result set but won't delete it. Use this if you are done with
		 * your query object but want to keep your result set around. The end result in the query object (and it's copies) is to
		 * have no result set associated with it.

		 */
		void relinquishResults() { m_sharedData->m_results=0; m_sharedData->m_flags &= ~SharedData::FLAG_SINGLE_RESULTS; }
		/** 
		 * @brief Relinquishes control over the parameter set.
		 *
		 * Calling this will disassociate the query object (and it's copies) from the parameter set but won't delete it. Use this if you are done with
		 * your query object but want to keep your parameter set around. The end result in the query object (and it's copies) is to
		 * have no parameter set associated with it.
		 */
		void relinquishParameters() { m_sharedData->m_parameters=0; m_sharedData->m_flags |= SharedData::FLAG_SINGLE_PARAMETERS; }

		/** 
		 * @brief Return a void pointer to the result set.
		 * 
		 * Usually you would keep the original typed pointer around but if you lost it for some reason you can use this and cast it.
		 */
		void* results() { return m_sharedData->m_results; }
		/** 
		 * @brief Return a void pointer to the parameter set.
		 * 
		 * Usually you would keep the original typed pointer around but if you lost it for some reason you can use this and cast it.
		 */
		void* parameters() { return m_sharedData->m_parameters; }

		/** 
		 * @brief Destroy the result set (if suppose to) and relinquish.
		 */
		void clearResults() { m_sharedData->destroyResults(); relinquishResults(); }
		/** 
		 * @brief Destroy the parameter set (if suppose to) relinquish.
		 */
		void clearParameters() { m_sharedData->destroyParameters(); relinquishParameters(); }

		/** 
		 * @brief Reset the query object as though you are making it anew from the default constructor.
		 *
		 * This is effectively like destroying the query object and creating a brand new one with the same name. Note that
		 * FLAG_ORIGINAL will be set after doing this.
		 */
		void reset() { m_sharedData.reset(new SharedData); m_flags=FLAG_ORIGINAL; keepAlive(false); }

		// Someday maybe
		/*void swap()
		{
			unsigned char flags(m_sharedData->m_flags);
			void* results(m_sharedData->m_results);
			m_sharedData->m_results = m_sharedData->m_parameters;
			m_sharedData->m_parameters = results;
		}*/
	};

	/** 
	 * @brief Build a series of queries into a transaction.
	 *
	 * By using this class to execute a series of queries you let the ASql engine handle insuring they
	 * are executed sequentially. Should an error or cancellation crop up in one query all changes made
	 * by preceding queries in the transaction will be rolled back and proceeding queries in the transaction
	 * will be dumped from the queue.
	 *
	 * Note that if an error or cancellation occurs in a query that has no callback function, proceeding queries
	 * in the transaction will be checked for callback functions and the first one found will be called.
	 *
	 * Note that the statement obviously MUST all be from the same connection.
	 */
	template<class T> class Transaction
	{
	public:
		/** 
		 * @brief Ties query objects to their statements.
		 */
		struct Item
		{
			Item(Query query, T* statement): m_query(query), m_statement(statement) {}
			Item(const Item& x): m_query(x.m_query), m_statement(x.m_statement) {}
			Query m_query;
			T* m_statement;
		};
	private:
		std::vector<Item> m_items;
	public:
		typedef typename std::vector<Item>::iterator iterator;
		/** 
		 * @brief Add a query to the transaction.
		 *
		 * This adds the query to the back of the line.
		 * 
		 * @param query Query to add.
		 * @param statement Associated statement.
		 */
		inline void push(Query& query, T& statement) { m_items.push_back(Item(query, &statement)); }
		/** 
		 * @brief Remove all queries from the transaction.
		 */
		inline void clear() { m_items.clear(); }
		/** 
		 * @brief Return a starting iterator.
		 */
		inline iterator begin() { return m_items.begin(); }
		/** 
		 * @brief Return a end iterator.
		 */
		inline iterator end() { return m_items.end(); }
		/** 
		 * @brief Return true if the transaction is empty.
		 */
		inline bool empty() { return m_items.size()==0; }

		/** 
		 * @brief Cancel all queries in the transaction.
		 */
		void cancel();

		/** 
		 * @brief Initiate the transaction.
		 */
		void start() { m_items.front().m_statement->connection.queue(*this); }
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

	/** 
	 * @brief Defines some functions and data types shared between ASql engines
	 */
	template<class T> class ConnectionPar: private Connection
	{
	private:
		struct QuerySet
		{
			QuerySet(Query& query, T* const& statement, const bool commit): m_query(query), m_statement(statement), m_commit(commit) {}
			QuerySet() {}
			Query m_query;
			bool m_commit;
			T* m_statement;
		};
		/** 
		 * @brief Thread safe queue of queries.
		 */
		class Queries: public std::queue<QuerySet>, public boost::mutex {} queries;

		/** 
		 * @brief Function that runs in threads.
		 */
		void intHandler();

		/** 
		 * @brief Locks the mutex on a statement and set's the canceller to the queries canceller
		 */
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
		/** 
		 * @brief Start all threads of the handler
		 */
		void start();
		/** 
		 * @brief Terminate all thread of the handler
		 */
		void terminate();
		/** 
		 * @brief Queue up a transaction for completion
		 */
		void queue(Transaction<T>& transaction);

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

	Queries intQueries;

	while(1)
	{
		terminateLock.lock();
		if(terminateBool)
			break;
		terminateLock.unlock();
		
		QuerySet querySet;

		if(intQueries.empty())
		{		
			queriesLock.lock();
			if(!queries.size())
			{
				wakeUp.wait(queriesLock);
				queriesLock.unlock();
				continue;
			}
			querySet=queries.front();
			while(!queries.front().m_commit)
			{
				queries.pop();
				intQueries.push(queries.front());
			}
			queries.pop();
			queriesLock.unlock();
		}
		else
		{
			querySet=intQueries.front();
			intQueries.pop();
		}

		Error error;

		querySet.m_statement->m_stop = &(querySet.m_query.m_sharedData->m_cancel);
		try
		{
			TakeStatement takeStatement(querySet.m_statement->m_stop, querySet.m_query.m_sharedData->m_cancel, querySet.m_statement->executeMutex);
			if(querySet.m_query.m_sharedData->m_flags & Query::SharedData::FLAG_SINGLE_PARAMETERS)
			{
				if(querySet.m_query.m_sharedData->m_flags & Query::SharedData::FLAG_SINGLE_RESULTS)
				{
					if(!querySet.m_statement->execute(static_cast<const Data::Set*>(querySet.m_query.parameters()), *static_cast<Data::Set*>(querySet.m_query.results()), false)) querySet.m_query.clearResults();
				}
				else
					querySet.m_statement->execute(static_cast<const Data::Set*>(querySet.m_query.parameters()), static_cast<Data::SetContainer*>(querySet.m_query.results()), querySet.m_query.m_sharedData->m_insertId, querySet.m_query.m_sharedData->m_rows, false);
			}
			else
			{
				querySet.m_statement->execute(*static_cast<const Data::SetContainer*>(querySet.m_query.parameters()), querySet.m_query.m_sharedData->m_rows, false);
			}

			if(querySet.m_commit)
				querySet.m_statement->commit();
		}
		catch(const Error& e)
		{
			querySet.m_query.m_sharedData->m_error=e;

			querySet.m_statement->rollback();

			queriesLock.lock();
			QuerySet tmpQuerySet=querySet;
			while(!intQueries.empty())
			{
				tmpQuerySet=intQueries.front();
				intQueries.pop();
				if(!querySet.m_query.isCallback() && tmpQuerySet.m_query.isCallback())
					querySet.m_query.setCallback(tmpQuerySet.m_query.getCallback());

			}
			queriesLock.unlock();
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
	queries.push(QuerySet(query, statement, true));
	wakeUp.notify_one();
}

template<class T> const bool ASql::ConnectionPar<T>::s_false = false;

template<class T> void ASql::ConnectionPar<T>::queue(Transaction<T>& transaction)
{
	boost::lock_guard<boost::mutex> queriesLock(queries);

	for(typename Transaction<T>::iterator it=transaction.begin(); it!=transaction.end(); ++it)
		queries.push(QuerySet(it->m_query, it->m_statement, false));
	queries.back().m_commit = true;

	wakeUp.notify_one();
}

template<class T> void ASql::Transaction<T>::cancel()
{
	for(iterator it=begin(); it!=end(); ++it)
		it->m_query.cancel();
}

#endif
