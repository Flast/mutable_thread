//          Copyright Kohei Takahashi 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// see http://open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3356.html

#ifndef MUTABLE_THREAD_DETAIL_N3356_HPP_
#define MUTABLE_THREAD_DETAIL_N3356_HPP_

#include <boost/assert.hpp>
#include <boost/move/move.hpp>
#include <boost/utility/swap.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/thread/exceptions.hpp>
#include <boost/system/error_code.hpp>
#include <mutable_thread/detail/exception.hpp>

namespace mutable_threads { namespace n3356 {

class mutable_thread : private boost::thread
{
    typedef mutable_thread          this_type;
    typedef boost::thread           thread_type;
    typedef boost::function<void()> function_type;

    BOOST_MOVABLE_BUT_NOT_COPYABLE(mutable_thread)

    struct thread_data_t
    {
        bool                      term;
        boost::mutex              mutex;
        boost::condition_variable cond;
        function_type             func;

        explicit
        thread_data_t() : term(false) {}

        template <typename F>
        explicit
        thread_data_t(F f)
          : term(false), func(f) {}
    }; // struct thread_data_t

    static void
    S_thraed_main_(boost::shared_ptr<thread_data_t> data, boost::barrier *barrier)
    {
        typedef boost::unique_lock<boost::mutex> lock_type;

        while (true)
        {
            lock_type guard(data->mutex);
            if (barrier)
            {
                barrier->wait();
                barrier = NULL;
            }
            if (data->term) { return; }
            while (data->func.empty())
            {
                if (data->term) { return; } // terminate
                data->cond.wait(guard);
            }

            data->func();
            data->func.clear();
        }
    }

public:
    mutable_thread()
      : m_data_(boost::make_shared<thread_data_t>())
    {
        boost::barrier b(2);
        thread_type(S_thraed_main_, m_data_, &b).swap(*this);
        b.wait();
    }

    mutable_thread(BOOST_RV_REF(mutable_thread) other) BOOST_NOEXCEPT
      : thread_type(boost::move(static_cast<thread_type &>(other)))
      , m_data_(move(other.m_data_)) {}

    template <typename F>
    explicit
    mutable_thread(F f)
      : m_data_(boost::make_shared<thread_data_t>(f))
    {
        boost::barrier b(2);
        thread_type(S_thraed_main_, m_data_, &b).swap(*this);
        b.wait();
    }

    mutable_thread &
    operator=(BOOST_RV_REF(mutable_thread) other) BOOST_NOEXCEPT
    {
        mutable_thread(boost::move(other)).swap(*this);
        return *this;
    }

    void
    join()
    {
        if (!joinable())
        {
            BOOST_THROW_EXCEPTION(
              detail::mutable_thread_exception("\"!joinable()\" failed"));
        }

        typedef boost::unique_lock<boost::mutex> lock_type;
        if (lock_type guard = lock_type(m_data_->mutex))
        {
            m_data_->term = true;
            m_data_->cond.notify_one();
        }

        try
        {
            thread_type::join();
            m_data_.reset();
        }
        catch (boost::thread_interrupted &e)
        {
            lock_type(m_data_->mutex),
            m_data_->term = false,
            throw;
        }
    }

    using thread_type::joinable;

    template <typename F>
    bool
    try_execute(F f)
    {
        typedef boost::unique_lock<boost::mutex> lock_type;

        if (is_done() || is_joining())
        {
            BOOST_THROW_EXCEPTION(
              boost::lock_error(
                boost::system::errc::operation_not_permitted
              , "\"!is_done() && !is_joining()\" failed"));
        }

        if (lock_type guard = lock_type(m_data_->mutex, boost::try_to_lock))
        {
            if (is_done() || is_joining()) { return false; }

            BOOST_ASSERT(m_data_->func.empty());
            m_data_->func = f;
            m_data_->cond.notify_one();
            return true;
        }
        return false;
    }

    template <typename F>
    bool
    execute(F f)
    {
        typedef boost::unique_lock<boost::mutex> lock_type;

        if (is_done() || is_joining())
        {
            BOOST_THROW_EXCEPTION(
              boost::lock_error(
                boost::system::errc::operation_not_permitted
              , "\"!is_done() && !is_joining()\" failed"));
        }

        if (lock_type guard = lock_type(m_data_->mutex))
        {
            if (is_done() || is_joining()) { return false; }

            BOOST_ASSERT(m_data_->func.empty());
            m_data_->func = f;
            m_data_->cond.notify_one();
            return true;
        }
        return false;
    }

    bool
    is_joining() const BOOST_NOEXCEPT
    {
        return m_data_ && m_data_->term;
    }

    bool
    is_done() const BOOST_NOEXCEPT
    {
        return !m_data_;
    }

    using thread_type::id;
    using thread_type::get_id;
    using thread_type::native_handle;

    void
    swap(mutable_thread &other) BOOST_NOEXCEPT
    {
        thread_type::swap(other);
        boost::swap(m_data_, other.m_data_);
    }

private:
    boost::shared_ptr<thread_data_t> m_data_;
}; // class mutable_thread

} } // namespace mutable_threads::n3356

#endif // MUTABLE_THREAD_DETAIL_N3356_HPP_

