#include "asql/asql.hpp"

ASql::QueryPar::SharedData::SharedData(const ResultType resultType, Data::Set* const parameters, void* const result, unsigned long long* const insertId, unsigned long long* const rows)
	:m_parameters(parameters), m_result(result), m_resultType(resultType), m_insertId(insertId), m_rows(rows), m_cancel(false), m_busy(false)
{
}

ASql::QueryPar::QueryPar(const ResultType resultType, Data::Set* const parameters, void* const result, unsigned long long* const insertId, unsigned long long* const rows)
	:m_isOriginal(true), m_keepAlive(false), m_sharedData(new SharedData(resultType, parameters, result, insertId, rows))
{
}

ASql::QueryPar::SharedData::~SharedData()
{
	delete m_parameters;
	delete m_insertId;
	delete m_rows;
	if(m_resultType == RESULT_TYPE_CONTAINER)
		delete static_cast<Data::SetContainerPar*>(m_result);
	else
		delete static_cast<Data::Set*>(m_result);
}

const bool ASql::Statement::s_false = false;
