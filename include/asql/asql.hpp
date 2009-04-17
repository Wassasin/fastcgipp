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
		};

		/** 
		 * @brief Base class to to SetContainer.
		 */
		struct SetContainerPar
		{
			virtual Set& manufacture() =0;
			virtual void trim() =0;
		};

		/** 
		 * @brief Container class for Set objects.
		 *
		 * This class defines a basic container for types derived from the Set class.
		 *	It is intended for retrieving multi-row results from SQL queries. In order
		 *	function the passed container type must have the following member functions:
		 *	push_back(), back(), pop_back().
		 *
		 *	@tparam Container type. Must be sequential.
		 */
		template<class T> class SetContainer: public SetContainerPar, public T
		{
			Set& manufacture() { T::push_back(typename T::value_type()); return T::back(); }
			void trim() { T::pop_back(); }
		};
	
		/** 
		 * @brief Container class for Set objects.
		 *
		 * This class defines container for types derived from the Set class that need
		 * to be stored in shared pointers. It is intended for retrieving multi-row results
		 * from SQL queries. In order function the passed container type must have the
		 * following member functions: push_back(), back(), pop_back().
		 *
		 * All data in it will be dynamically allocated into a boost::shared_ptr
		 *
		 *	@tparam Container type. Must be sequential.
		 */
		template<class T> class SharedSetContainer: public SetContainerPar, public T
		{
			Set& manufacture() { T::push_back(boost::shared_ptr<typename T::value_type>(new typename T::value_type)); return *T::back(); }
			void trim() { T::pop_back(); }
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

	class QueryPar
	{
	public:
		enum ResultType { RESULT_TYPE_SINGLE, RESULT_TYPE_CONTAINER };

	protected:
		QueryPar(const ResultType resultType, Data::Set* const parameters, void* const result, unsigned long long* const insertId, unsigned long long* const rows);
		bool m_isOriginal;
		bool m_keepAlive;

		struct SharedData
		{
			inline SharedData(const ResultType resultType, Data::Set* const parameters, void* const result, unsigned long long* const insertId, unsigned long long* const rows);
			~SharedData();
			Data::Set* const m_parameters;
			void* const m_result;
			const ResultType m_resultType;
			unsigned long long int* const m_insertId;
			unsigned long long int* const m_rows;
			Error m_error;
			boost::function<void()> m_callback;
			boost::mutex m_callbackMutex;
			bool m_cancel;
			bool m_busy;
		};

		boost::shared_ptr<SharedData> m_sharedData;

	public:
		inline QueryPar(const QueryPar& x): m_isOriginal(false), m_sharedData(x.m_sharedData) {}
		inline ~QueryPar() { if(m_isOriginal && !m_keepAlive) cancel(); }
		long int insertId() const { return m_sharedData->m_insertId?*(m_sharedData->m_insertId):-1; }
		long int rows() const { return m_sharedData->m_rows?*(m_sharedData->m_rows):-1; }
		void setCallback(boost::function<void()> callback) { m_sharedData->m_callback = callback; }
		bool busy() const { return m_sharedData->m_busy; }
		void cancel() { boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex); m_sharedData->m_cancel = true; }
		Error error() const { return m_sharedData->m_error; }
		void keepAlive() { m_keepAlive=true; }

	private:
		inline void callback();
		template<class T> friend class ConnectionPar;

	};

	template<class PARAMETERS> class QueryParametersOnly: public QueryPar
	{
	public:
		QueryParametersOnly(bool fetchLastInsertId=false)
			:QueryPar(RESULT_TYPE_CONTAINER, new PARAMETERS, 0, fetchLastInsertId?(new unsigned long long):0, 0) {}
		PARAMETERS& parameters(){ return *(PARAMETERS*)m_sharedData->m_parameters; }
	private:
		QueryParametersOnly(const QueryParametersOnly& x);
	};

	template<class RESULT> class QueryResultOnly: public QueryPar
	{
	public:
		QueryResultOnly(ResultType resultType=RESULT_TYPE_CONTAINER, bool fetchNumRows=false)
			:QueryPar(resultType, 0, new RESULT, 0, fetchNumRows?(new unsigned long long):0) {}
		RESULT& result(){ return *(RESULT*)m_sharedData->m_result; }
	private:
		QueryResultOnly(const QueryResultOnly& x);
	};

	template<class PARAMETERS, class RESULT> class Query: public QueryPar
	{
	public:
		Query(ResultType resultType=RESULT_TYPE_CONTAINER, bool fetchNumRows=false)
			:QueryPar(resultType, new PARAMETERS, new RESULT, 0, fetchNumRows?(new unsigned long long):0) {}
		PARAMETERS& parameters(){ return *(PARAMETERS*)m_sharedData->m_parameters; }
		RESULT& result(){ return *(RESULT*)m_sharedData->m_result; }
	private:
		Query(const Query& x);
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
			QuerySet(QueryPar& query, T* const& statement): m_query(query), m_statement(statement) {}
			QueryPar m_query;
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

	protected:
		ConnectionPar(const int maxThreads_): Connection(maxThreads_) {}
		inline void queue(T* const& statement, QueryPar& query);
	public:
		void start();
		void terminate();
	};	

	/** 
	 * @brief SQL %Statement.
	 */
	class Statement
	{
	protected:
		Data::Conversions paramsConversions;
		Data::Conversions resultsConversions;
		static const bool s_false;
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
			if(querySet.m_query.m_sharedData->m_resultType == QueryPar::RESULT_TYPE_CONTAINER)
				querySet.m_statement->execute(querySet.m_query.m_sharedData->m_parameters, static_cast<Data::SetContainerPar*>(querySet.m_query.m_sharedData->m_result), querySet.m_query.m_sharedData->m_insertId, querySet.m_query.m_sharedData->m_rows);
			else
				querySet.m_statement->execute(querySet.m_query.m_sharedData->m_parameters, *static_cast<Data::Set*>(querySet.m_query.m_sharedData->m_result));
		}
		catch(const Error& e)
		{
			querySet.m_query.m_sharedData->m_error=e;
		}
		querySet.m_statement->m_stop = &T::s_false;

		querySet.m_query.callback();
		querySet.m_query.m_sharedData->m_busy = false;
	}

	{
		boost::lock_guard<boost::mutex> threadsLock(threadsMutex);
		--threads;
	}
	threadsChanged.notify_one();
}

template<class T> void ASql::ConnectionPar<T>::queue(T* const& statement, QueryPar& query)
{
	query.m_sharedData->m_busy = true;
	boost::lock_guard<boost::mutex> queriesLock(queries);
	queries.push(QuerySet(query, statement));
	wakeUp.notify_one();
}

void ASql::QueryPar::callback()
{
	if(!m_sharedData->m_cancel && !m_sharedData->m_callback.empty())
	{
		boost::lock_guard<boost::mutex> lock(m_sharedData->m_callbackMutex);
		m_sharedData->m_callback();
	}
}

#endif
