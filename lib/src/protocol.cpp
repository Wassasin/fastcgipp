/***********************************************************************
* Copyright (C) 2007 Eddie                                             *
*                                                                      *
* This file is part of fastcgi++.                                      *
*                                                                      *
* fastcgi++ is free software: you can redistribute it and/or modify    *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or    *
* (at your option) any later version.                                  *
*                                                                      *
* fastcgi++ is distributed in the hope that it will be useful,         *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU General Public License for more details.                         *
*                                                                      *
* You should have received a copy of the GNU General Public License    *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.   *
************************************************************************/


#include <fastcgi++/protocol.hpp>

void Fastcgi::Protocol::processParamHeader(const char* data, const char*& name, size_t& nameSize, const char*& value, size_t& valueSize)
{
	if(*data>>7)
	{
		nameSize=readBigEndian(*(uint32_t*)data) & 0x7fffffff;
		data+=sizeof(uint32_t);
	}
	else nameSize=*data++;

	if(*data>>7)
	{
		valueSize=readBigEndian(*(uint32_t*)data) & 0x7fffffff;
		data+=sizeof(uint32_t);
	}
	else valueSize=*data++;
	name=data;
	value=name+nameSize;
}

Fastcgi::Protocol::ManagementReply<14, 2, 8> Fastcgi::Protocol::maxConnsReply("FCGI_MAX_CONNS", "10");
Fastcgi::Protocol::ManagementReply<13, 2, 1> Fastcgi::Protocol::maxReqsReply("FCGI_MAX_REQS", "50");
Fastcgi::Protocol::ManagementReply<15, 1, 8> Fastcgi::Protocol::mpxsConnsReply("FCGI_MPXS_CONNS", "1");
