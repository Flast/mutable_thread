#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include <boost/test/minimal.hpp>

#include <boost/mutable_thread/mutable_thread.hpp>

typedef boost::unique_lock<boost::mutex> locker;

boost::mutex mtx;
bool flag = false;

void f()
{
    flag = true;
    mtx.unlock();
}

int test_main(int, char **)
{
    mtx.lock();
    boost::mutable_threads::mutable_thread mt(f);
    static_cast<void>(locker(mtx)), static_cast<void>(BOOST_REQUIRE(flag));
    mt.join();
    return 0;
}

