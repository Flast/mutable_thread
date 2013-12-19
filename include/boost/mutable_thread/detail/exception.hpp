//          Copyright Kohei Takahashi 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MUTABLE_THREAD_DETAIL_EXCEPTION_HPP
#define BOOST_MUTABLE_THREAD_DETAIL_EXCEPTION_HPP

#include <string>
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace boost { namespace mutable_threads { namespace detail {

struct mutable_thread_exception : public std::logic_error
{
    explicit
    mutable_thread_exception(const std::string &message)
      : std::logic_error(message)
    {
    }
}; // struct mutable_thread_exception

} } } // namespace boost::mutable_threads::detail

#endif // BOOST_MUTABLE_THREAD_DETAIL_EXCEPTION_HPP

