//          Copyright Kohei Takahashi 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

    static void
    S_thraed_main_(mutable_thread *this_, boost::barrier *barrier)
    {
        boost::shared_ptr<bool>                      term = this_->m_term_;
        boost::shared_ptr<boost::mutex>              mutex = this_->m_mutex_;
        boost::shared_ptr<boost::condition_variable> cond = this_->m_cond_;
        boost::shared_ptr<function_type>             func = this_->m_func_;

        typedef boost::unique_lock<boost::mutex> lock_type;

        while (true)
        {
            lock_type guard(*mutex);
            if (barrier)
            {
                barrier->wait();
                barrier = NULL;
            }
            while (func->empty())
            {
                if (*term) { return; } // terminate
                cond->wait(guard);
            }

            (*func)();
            func->clear();
        }
    }

public:
    mutable_thread()
      : m_term_(boost::make_shared<bool>(false))
      , m_mutex_(boost::make_shared<boost::mutex>())
      , m_cond_(boost::make_shared<boost::condition_variable>())
      , m_func_(boost::make_shared<function_type>())
    {
        boost::barrier b(2);
        thread_type(S_thraed_main_, this, &b).swap(*this);
        b.wait();
    }

    mutable_thread(BOOST_RV_REF(mutable_thread) other) BOOST_NOEXCEPT
      : thread_type(boost::move(static_cast<thread_type &>(other)))
      , m_term_(boost::move(other.m_term_))
      , m_mutex_(boost::move(other.m_mutex_))
      , m_cond_(boost::move(other.m_cond_))
      , m_func_(boost::move(other.m_func_))
    {
        other.m_term_.reset();
        other.m_mutex_.reset();
        other.m_cond_.reset();
        other.m_func_.reset();
    }

    template <typename F>
    explicit
    mutable_thread(F f)
      : m_term_(boost::make_shared<bool>(false))
      , m_mutex_(boost::make_shared<boost::mutex>())
      , m_cond_(boost::make_shared<boost::condition_variable>())
      , m_func_(boost::make_shared<function_type>(f))
    {
        boost::barrier b(2);
        thread_type(S_thraed_main_, this, &b).swap(*this);
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
        if (lock_type guard = lock_type(*m_mutex_))
        {
            *m_term_ = true;
            m_cond_->notify_one();

            m_mutex_.reset();
            m_cond_.reset();
            m_func_.reset();
        }
        thread_type::join();
        m_term_.reset();
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

        if (lock_type guard = lock_type(*m_mutex_, boost::try_to_lock))
        {
            if (is_done() || is_joining()) { return false; }

            BOOST_ASSERT(m_func_->empty());
            *m_func_ = f;
            m_cond_->notify_one();
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

        if (lock_type guard = lock_type(*m_mutex_))
        {
            if (is_done() || is_joining()) { return false; }

            BOOST_ASSERT(m_func_->empty());
            *m_func_ = f;
            m_cond_->notify_one();
            return true;
        }
        return false;
    }

    bool
    is_joining() const BOOST_NOEXCEPT
    {
        return !m_mutex_ && !m_cond_ && !m_func_ && joinable();
    }

    bool
    is_done() const BOOST_NOEXCEPT
    {
        return !m_mutex_ && !m_cond_ && !m_func_ && !joinable();
    }

    using thread_type::id;
    using thread_type::get_id;
    using thread_type::native_handle;

    void
    swap(mutable_thread &other) BOOST_NOEXCEPT
    {
        thread_type::swap(other);
        boost::swap(m_term_ , other.m_term_ );
        boost::swap(m_mutex_, other.m_mutex_);
        boost::swap(m_cond_ , other.m_cond_ );
        boost::swap(m_func_ , other.m_func_ );
    }

private:
    boost::shared_ptr<bool>                      m_term_;
    boost::shared_ptr<boost::mutex>              m_mutex_;
    boost::shared_ptr<boost::condition_variable> m_cond_;
    boost::shared_ptr<function_type>             m_func_;
}; // class mutable_thread

} } // namespace mutable_threads::n3356

#endif // MUTABLE_THREAD_DETAIL_N3356_HPP_

