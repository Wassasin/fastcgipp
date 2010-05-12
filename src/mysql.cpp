//! \file mysql.cpp Defines functions and data for handling MySQL queries
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


#include <asql/mysql.hpp>
#include <utf8_codecvt.hpp>

void ASql::MySQL::Connection::connect(const char* host, const char* user, const char* passwd, const char* db, unsigned int port, const char* unix_socket, unsigned long client_flag, const char* const charset)
{
	if(m_initialized)
	{
		mysql_stmt_close(foundRowsStatement);
		m_initialized = false;
	}

	if(!mysql_init(&connection))
		throw Error(&connection);

	if(!mysql_real_connect(&connection, host, user, passwd, db, port, unix_socket, client_flag))
		throw Error(&connection);

	if(mysql_set_character_set(&connection, charset))
		throw Error(&connection);
	
	if(mysql_autocommit(&connection, 0))
		throw Error(&connection);
	
	if(!(foundRowsStatement = mysql_stmt_init(&connection)))
		throw Error(&connection);

	if(mysql_stmt_prepare(foundRowsStatement, "SELECT FOUND_ROWS()", 19))
		throw Error(foundRowsStatement);

	std::memset(&foundRowsBinding, 0, sizeof(MYSQL_BIND));
	foundRowsBinding.buffer_type = MYSQL_TYPE_LONGLONG;
	foundRowsBinding.is_unsigned = 1;

	m_initialized = true;
}

ASql::MySQL::Connection::~Connection()
{
	if(m_initialized) mysql_close(&connection);
}

void ASql::MySQL::Connection::getFoundRows(unsigned long long* const& rows)
{
	if(mysql_stmt_bind_param(foundRowsStatement, 0))
		throw Error(foundRowsStatement);
	
	if(mysql_stmt_execute(foundRowsStatement))
		throw Error(foundRowsStatement);

	foundRowsBinding.buffer = rows;
	if(mysql_stmt_bind_result(foundRowsStatement, &foundRowsBinding))
		throw Error(foundRowsStatement);

	if(mysql_stmt_fetch(foundRowsStatement))
		throw Error(foundRowsStatement);
	mysql_stmt_free_result(foundRowsStatement);
	mysql_stmt_reset(foundRowsStatement);

}

void ASql::MySQL::Statement::init(const char* const& queryString, const size_t& queryLength, const Data::Set* const parameterSet, const Data::Set* const resultSet)
{
	if(m_initialized)
	{
		mysql_stmt_close(stmt);
		m_initialized = false;
	}
	
	stmt=mysql_stmt_init(&connection.connection);
	if(!stmt)
		throw Error(&connection.connection);

	if(mysql_stmt_prepare(stmt, queryString, queryLength))
		throw Error(stmt);

	if(parameterSet) buildBindings(stmt, *parameterSet, paramsConversions, paramsBindings);
	if(resultSet) buildBindings(stmt, *resultSet, resultsConversions, resultsBindings);
}

void ASql::MySQL::Statement::executeParameters(const Data::Set* const& parameters)
{
	if(parameters)
	{
		bindBindings(*const_cast<Data::Set*>(parameters), paramsConversions, paramsBindings);
		for(Data::Conversions::iterator it=paramsConversions.begin(); it!=paramsConversions.end(); ++it)
			it->second->convertParam();
		if(mysql_stmt_bind_param(stmt, paramsBindings.get())!=0) throw Error(stmt);
	}

	if(mysql_stmt_execute(stmt)!=0) throw Error(stmt);
}

bool ASql::MySQL::Statement::executeResult(Data::Set& row)
{
	bindBindings(row, resultsConversions, resultsBindings);
	if(mysql_stmt_bind_result(stmt, resultsBindings.get())!=0) throw Error(stmt);
	switch (mysql_stmt_fetch(stmt))
	{
	case 1:
		throw Error(stmt);
	case MYSQL_NO_DATA:
		return false;
	default:
		for(Data::Conversions::iterator it=resultsConversions.begin(); it!=resultsConversions.end(); ++it)
			it->second->convertResult();
		return true;
	};
}

void ASql::MySQL::Statement::execute(const Data::Set* const parameters, Data::SetContainer* const results, unsigned long long int* const insertId, unsigned long long int* const rows, bool docommit)
{
	if(*m_stop) goto end;
	executeParameters(parameters);

	if(results)
	{
		Data::SetContainer& res=*results;

		while(1)
		{{
			Data::Set& row=res.manufacture();
			bindBindings(row, resultsConversions, resultsBindings);
			if(!executeResult(row))
			{
				res.trim();
				break;
			}
			if(*m_stop)
			{
				res.trim();
				goto end;
			}
		}}

		if(*m_stop) goto end;
		if(rows) connection.getFoundRows(rows);
	}
	else
	{
		if(*m_stop) goto end;
		if(rows) *rows = mysql_stmt_affected_rows(stmt);
		if(*m_stop) goto end;
		if(insertId) *insertId = mysql_stmt_insert_id(stmt);
	}

end:
	if(*m_stop)
		rollback();
	else if(docommit)
		commit();
	mysql_stmt_free_result(stmt);
	mysql_stmt_reset(stmt);
}

bool ASql::MySQL::Statement::execute(const Data::Set* const parameters, Data::Set& results, bool docommit)
{
	bool retval(false);
	if(*m_stop) goto end;
	executeParameters(parameters);
	if(*m_stop) goto end;
	retval=executeResult(results);
end:
	if(*m_stop)
		rollback();
	else if(docommit)
		commit();
	mysql_stmt_free_result(stmt);
	mysql_stmt_reset(stmt);
	return retval;
}

void ASql::MySQL::Statement::execute(const Data::SetContainer& parameters, unsigned long long int* rows, bool docommit)
{
	if(rows) *rows = 0;
	
	for(const Data::Set* set=parameters.pull(); set!=0; set=parameters.pull())
	{
		if(*m_stop) break;
		executeParameters(set);
		if(*m_stop) break;
		if(rows) *rows += mysql_stmt_affected_rows(stmt);
	}
	if(*m_stop)
		rollback();
	else if(docommit)
		commit();
	mysql_stmt_free_result(stmt);
	mysql_stmt_reset(stmt);
}

void ASql::MySQL::Statement::buildBindings(MYSQL_STMT* const& stmt, const ASql::Data::Set& set, ASql::Data::Conversions& conversions, boost::scoped_array<MYSQL_BIND>& bindings)
{
	using namespace Data;
	
	conversions.clear();

	const int& bindSize=set.numberOfSqlElements();
	if(!bindSize) return;
	bindings.reset(new MYSQL_BIND[bindSize]);

	std::memset(bindings.get(), 0, sizeof(MYSQL_BIND)*bindSize);

	for(int i=0; i<bindSize; ++i)
	{
		Index element(set.getSqlIndex(i));

		// Handle NULL
		if(element.type>=U_TINY_N)
			element.type=Type(element.type-U_TINY_N);	// Make it the same type without the nullableness

		// Handle unsigned
		if(element.type<=U_BIGINT)
		{
			bindings[i].is_unsigned=1;
			element.type=Type(element.type+TINY);
		}

		// Start decoding values
		switch(element.type)
		{
			case TINY:
			{
				bindings[i].buffer_type=MYSQL_TYPE_TINY;
				break;
			}

			case SHORT:
			{
				bindings[i].buffer_type=MYSQL_TYPE_SHORT;
				break;
			}
			
			case INT:
			{
				bindings[i].buffer_type=MYSQL_TYPE_LONG;
				break;
			}

			case BIGINT:
			{
				bindings[i].buffer_type=MYSQL_TYPE_LONGLONG;
				break;
			}

			case FLOAT:
			{
				bindings[i].buffer_type=MYSQL_TYPE_FLOAT;
				break;
			}

			case DOUBLE:
			{
				bindings[i].buffer_type=MYSQL_TYPE_DOUBLE;
				break;
			}

			case DATE:
			{
				TypedConversion<Date>* conv = new TypedConversion<Date>;
				bindings[i].buffer = &conv->internal;
				bindings[i].buffer_type = MYSQL_TYPE_DATE;
				conversions[i].reset(conv);
				break;
			}

			case DATETIME:
			{
				TypedConversion<Datetime>* conv = new TypedConversion<Datetime>;
				bindings[i].buffer = &conv->internal;
				bindings[i].buffer_type = MYSQL_TYPE_DATETIME;
				conversions[i].reset(conv);
				break;
			}

			case TIME:
			{
				TypedConversion<Time>* conv = new TypedConversion<Time>;
				bindings[i].buffer = &conv->internal;
				bindings[i].buffer_type = MYSQL_TYPE_TIME;
				conversions[i].reset(conv);
				break;
			}

			case BLOB:
			{
				TypedConversion<Blob>* conv = new TypedConversion<Blob>(i, stmt, MYSQL_TYPE_BLOB, bindings[i].buffer);
				bindings[i].length = &conv->length;
				bindings[i].buffer_type = conv->bufferType;
				conversions[i].reset(conv);
				break;
			}

			case TEXT:
			{
				TypedConversion<Text>* conv = new TypedConversion<Text>(i, stmt, MYSQL_TYPE_STRING, bindings[i].buffer);
				bindings[i].length = &conv->length;
				bindings[i].buffer_type = conv->bufferType;
				conversions[i].reset(conv);
				break;
			}

			case WTEXT:
			{
				TypedConversion<Wtext>* conv = new TypedConversion<Wtext>(i, stmt, bindings[i].buffer);

				bindings[i].length = &conv->length;
				bindings[i].buffer_type = conv->bufferType;
				conversions[i].reset(conv);
				break;
			}

			case CHAR:
			case BINARY:
			{
				bindings[i].buffer_length = element.size;
				bindings[i].buffer_type = element.type==CHAR?MYSQL_TYPE_STRING:MYSQL_TYPE_BLOB;
				break;
			}

			default:
			{
				// Invalid element type, this shouldn't happen
				break;
			}
		}
	}
}

void ASql::MySQL::Statement::bindBindings(Data::Set& set, Data::Conversions& conversions, boost::scoped_array<MYSQL_BIND>& bindings)
{
	int bindSize=set.numberOfSqlElements();
	for(int i=0; i<bindSize; ++i)
	{
		Data::Index element(set.getSqlIndex(i));
		if(element.type >= Data::U_TINY_N)
		{
			bindings[i].is_null = (my_bool*)&((Data::NullablePar*)element.data)->nullness;
			element.data = ((Data::NullablePar*)element.data)->getVoid();
		}

		Data::Conversions::iterator it=conversions.find(i);
		if(it==conversions.end())
			bindings[i].buffer=element.data;
		else
		{
			it->second->external=element.data;
			bindings[i].buffer=it->second->getPointer();
		}
	}
}

void ASql::MySQL::TypedConversion<ASql::Data::Datetime>::convertResult()
{
	*(boost::posix_time::ptime*)external=boost::posix_time::ptime(boost::gregorian::date(internal.year, internal.month, internal.day), boost::posix_time::time_duration(internal.hour, internal.minute, internal.second));
}

void ASql::MySQL::TypedConversion<ASql::Data::Datetime>::convertParam()
{
	std::memset(&internal, 0, sizeof(MYSQL_TIME));
	internal.year = ((boost::posix_time::ptime*)external)->date().year();
	internal.month = ((boost::posix_time::ptime*)external)->date().month();
	internal.day = ((boost::posix_time::ptime*)external)->date().day();
	internal.hour = ((boost::posix_time::ptime*)external)->time_of_day().hours();
	internal.minute = ((boost::posix_time::ptime*)external)->time_of_day().minutes();
	internal.second = ((boost::posix_time::ptime*)external)->time_of_day().seconds();
}

void ASql::MySQL::TypedConversion<ASql::Data::Date>::convertResult()
{
	*(boost::gregorian::date*)external=boost::gregorian::date(internal.year, internal.month, internal.day);
}

void ASql::MySQL::TypedConversion<ASql::Data::Date>::convertParam()
{
	std::memset(&internal, 0, sizeof(MYSQL_TIME));
	internal.year = ((boost::gregorian::date*)external)->year();
	internal.month = ((boost::gregorian::date*)external)->month();
	internal.day = ((boost::gregorian::date*)external)->day();
}

void ASql::MySQL::TypedConversion<ASql::Data::Time>::convertResult()
{
	*(boost::posix_time::time_duration*)external = boost::posix_time::time_duration(internal.neg?internal.hour*-1:internal.hour, internal.minute, internal.second);
}

void ASql::MySQL::TypedConversion<ASql::Data::Time>::convertParam()
{
	std::memset(&internal, 0, sizeof(MYSQL_TIME));
	internal.hour = std::abs(((boost::posix_time::time_duration*)external)->hours());
	internal.minute = std::abs(((boost::posix_time::time_duration*)external)->minutes());
	internal.second = std::abs(((boost::posix_time::time_duration*)external)->seconds());
	internal.neg = ((boost::posix_time::time_duration*)external)->hours() < 0 ? 1:0;
}

template void ASql::MySQL::TypedConversion<ASql::Data::Blob>::grabIt(ASql::Data::Blob& data);
template void ASql::MySQL::TypedConversion<ASql::Data::Text>::grabIt(ASql::Data::Text& data);
template<class T> void ASql::MySQL::TypedConversion<T>::grabIt(T& data)
{
	if(data.size() != length) data.resize(length);

	if(length)
	{
		MYSQL_BIND bind;
		std::memset(&bind, 0, sizeof(bind));
		bind.buffer=&data[0];
		bind.buffer_length=length;
		bind.length=&length;
		bind.buffer_type=bufferType;
		if(mysql_stmt_fetch_column(statement, &bind, column, 0)!=0) throw Error(statement);
	}
}

template void ASql::MySQL::TypedConversion<ASql::Data::Blob>::convertParam();
template void ASql::MySQL::TypedConversion<ASql::Data::Text>::convertParam();
template<class T> void ASql::MySQL::TypedConversion<T>::convertParam()
{
	T& data = *(T*)external;

	length = data.size();
	buffer = &data[0];
}

void ASql::MySQL::TypedConversion<ASql::Data::Wtext>::convertResult()
{
	using namespace std;
	
	vector<char>& conversionBuffer = inputBuffer;
	grabIt(conversionBuffer);

	wstring& output = *(wstring*)external;
	output.resize(conversionBuffer.size());

	if(conversionBuffer.size())
	{
		wchar_t* it;
		const char* tmp;
		mbstate_t conversionState = mbstate_t();
		if(use_facet<codecvt<wchar_t, char, mbstate_t> >(locale(locale::classic(), new utf8CodeCvt::utf8_codecvt_facet)).in(conversionState, (const char*)&conversionBuffer.front(), (const char*)&conversionBuffer.front() + conversionBuffer.size(), tmp, &output[0], &output[0] + output.size(), it)!=codecvt_base::ok)
			throw ASql::Error(CodeConversionErrorMsg, -1);
		output.resize(it-&output[0]);
		conversionBuffer.clear();
	}
}

void ASql::MySQL::TypedConversion<ASql::Data::Wtext>::convertParam()
{
	using namespace std;

	wstring& data = *(wstring*)external;

	inputBuffer.resize(data.size()*sizeof(wchar_t));

	if(inputBuffer.size())
	{
		const wchar_t* tmp;
		char* it;
		mbstate_t conversionState = mbstate_t();
		if(use_facet<codecvt<wchar_t, char, mbstate_t> >(locale(locale::classic(), new utf8CodeCvt::utf8_codecvt_facet)).out(conversionState, (const wchar_t*)&data[0], (const wchar_t*)&data[0] + data.size(), tmp, &inputBuffer.front(), &inputBuffer.front() + inputBuffer.size(), it)!=codecvt_base::ok) throw ASql::Error(CodeConversionErrorMsg, -1);
		inputBuffer.resize(it-&inputBuffer[0]);
	}

	buffer=&inputBuffer.front();
	length = inputBuffer.size();
}


//Instance the ConnectionPar functions
template void ASql::ConnectionPar<ASql::MySQL::Statement>::start();
template void ASql::ConnectionPar<ASql::MySQL::Statement>::terminate();
template void ASql::ConnectionPar<ASql::MySQL::Statement>::intHandler();
template void ASql::ConnectionPar<ASql::MySQL::Statement>::queue(ASql::MySQL::Statement* const& statement, Query& query);
template void ASql::ConnectionPar<ASql::MySQL::Statement>::queue(Transaction<ASql::MySQL::Statement>& transaction);
template void ASql::Transaction<ASql::MySQL::Statement>::cancel();


ASql::MySQL::Error::Error(MYSQL* mysql): ASql::Error(mysql_error(mysql), mysql_errno(mysql)) { }
ASql::MySQL::Error::Error(MYSQL_STMT* stmt): ASql::Error(mysql_stmt_error(stmt), mysql_stmt_errno(stmt)) { }

const char ASql::MySQL::CodeConversionErrorMsg[]="Error in code conversion to/from MySQL server.";
