//! \file asql.hpp Define a function to bind fastcgi++ to asql
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

#ifndef ASQLBIND_HPP
#define ASQLBIND_HPP

#include <cstring>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <fastcgi++/message.hpp>
#include <asql/asql.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	void asqlbinder__(ASql::Error error, const boost::function<void(Fastcgipp::Message)>& callback, int type);

	inline boost::function<void(ASql::Error)> asqlbind(const boost::function<void(Fastcgipp::Message)>& callback, const int type=1)
	{
		return boost::bind(asqlbinder__, _1, callback, type);
	}
}

#endif
