#ifndef ASQLDATA_HPP
#define ASQLDATA_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ostream>
#include <vector>
#include <string>
#include <map>

namespace ASql
{
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
			Nullable<T>& operator=(const T& x) { object=x; nullness=false; return *this; }
			Nullable(): NullablePar(true) {}
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
		typedef bool Boolean;
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
		typedef Nullable<bool> BooleanN;
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
			Index(const Boolean& x): type(U_TINY), data(const_cast<Boolean*>(&x)) { }
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
			Index(const BooleanN& x): type(U_TINY_N), data(const_cast<BooleanN*>(&x)) { }
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
}

#endif
