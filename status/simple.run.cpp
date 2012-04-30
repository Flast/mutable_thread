#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include <boost/test/minimal.hpp>

#include <mutable_thread/mutable_thread.hpp>

typedef boost::unique_lock<boost::mutex> locker;

boost::mutex mtx;
bool flag = false;

void f()
{
    static_cast<void>(locker(mtx)), static_cast<void>(flag = true);
}

int test_main(int, char **)
{
    mutable_threads::mutable_thread mt(f);
    static_cast<void>(locker(mtx)), static_cast<void>(BOOST_REQUIRE(flag));
    mt.join();
    return 0;
}

