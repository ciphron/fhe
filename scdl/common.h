/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMON_H
#define COMMON_H

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


template <class T>
T read_numeric(std::string str)
{
    T value;

    try {
        boost::trim(str);
        value = boost::lexical_cast<T>(str);
    }
    catch (const boost::bad_lexical_cast &) {
        throw "Could not parse numeric value";
    }

    return value;
}


#endif // COMMON_H
