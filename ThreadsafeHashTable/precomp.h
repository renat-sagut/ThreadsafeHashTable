#pragma once

#include <array>
#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <atomic>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>

#define BOOST_TEST_MODULE ThreadsafeHashTable
#include <boost/test/unit_test.hpp>