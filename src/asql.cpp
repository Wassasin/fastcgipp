#include "asql/asql.hpp"


ASql::Query::SharedData::SharedData(unsigned char flags)
	:m_parameters(0), m_result(0), m_insertId(0), m_rows(0), m_cancel(false), m_flags(flags)
{
}

ASql::Query::Query(unsigned char flags)
	:m_sharedData(new SharedData(flags)), m_flags(FLAG_ORIGINAL)
{
}

ASql::Query::SharedData::~SharedData()
{
	delete m_rows;
	delete m_insertId;
	
	if(m_flags&FLAG_SINGLE_RESULT)
		delete static_cast<Data::Set*>(m_result);
	else
		delete static_cast<Data::SetContainerPar*>(m_result);
	
	if(m_flags&FLAG_SINGLE_PARAMETER)
		delete static_cast<Data::Set*>(m_parameters);
	else
		delete static_cast<Data::SetContainerPar*>(m_parameters);
	
}

const bool ASql::Statement::s_false = false;
