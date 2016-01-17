#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <time.h>
#include <condition_variable>
#include <random>

#include "SlimReaderWriterLock.h"
#include "ThreadsafeHashTable.h"

#ifndef WIN32
#include <sys/time.h>
#else
#include <WinSock2.h>

int gettimeofday( struct timeval* tv, void* tz )
{
   union
   {
      int64_t ns100; // since 1.1.1601 in 100ns units
      FILETIME ft;
   } now;

   GetSystemTimeAsFileTime( &now.ft );

   tv->tv_usec = ( long )( ( now.ns100 / 10LL ) % 1000000LL );
   tv->tv_sec = ( long )( ( now.ns100 - 116444736000000000LL ) / 10000000LL );

   return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief seconds with microsecond resolution
////////////////////////////////////////////////////////////////////////////////
double TRI_microtime ()
{
   struct timeval t;

   gettimeofday( &t, 0 );

   return ( t.tv_sec ) + ( t.tv_usec / 1000000.0 );
}

size_t const lock_count = 11;

long const distrib_min = 0;
long const distrib_max = 1000000;

size_t const iter_count = 1000000;
size_t const initial_size = 10000;

size_t const reader_count = 2;
size_t const updater_count = 2;
size_t const inserter_count = 4;

typedef int TKey;
typedef int TValue;
typedef std::pair<TKey, TValue> TKeyValue;
//typedef kvs::ThreadsafeHashTable<TKey, TValue, lock_count, std::mutex> TConcurrentMap;
//typedef kvs::ThreadsafeHashTable<TKey, TValue, lock_count, boost::shared_mutex> TConcurrentMap;
typedef kvs::ThreadsafeHashTable<TKey, TValue, lock_count, kvs::SlimReaderWriterLock> TConcurrentMap;
typedef std::map<TKey, TValue> TSerialMap;

#pragma region concurrent_func

void ReadFunc( TConcurrentMap& concurrent_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      TValue current;
      concurrent_map.Find( distribution( generator ), current );
   }
};

void UpdateFunc( TConcurrentMap& concurrent_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );
      concurrent_map.Update( kv );
   }
};

void InsertFunc( TConcurrentMap& concurrent_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );
      concurrent_map.Insert( kv );
   }
};

#pragma endregion concurrent_func

#pragma region serial_func

void ReadSerial( TSerialMap& serial_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      serial_map.find( distribution( generator ) );
   }
};

void UpdateSerial( TSerialMap& serial_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );

      auto it = serial_map.find( kv.first );
      if ( it != serial_map.end() )
      {
         it->second = kv.second;
      }
   }
};

void InsertSerial( TSerialMap& serial_map, long const count )
{
   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );

   for ( long i = 0; i < count; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );

      serial_map.insert( kv );
   }
};

#pragma endregion serial_func

int main()
{
   // количество потоков
   int num_threads = reader_count + updater_count + inserter_count;

   TConcurrentMap concurrent_map;
   // заранее резервируем ячейки, чтобы не тратить время на рехэш
   concurrent_map.Reserve( iter_count * inserter_count * 2 );

   std::random_device rd;
   std::default_random_engine generator( rd() );
   std::uniform_int_distribution<int> distribution( distrib_min, distrib_max );
   for ( size_t i = 0; i < initial_size; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );

      // начальные данные
      concurrent_map.Insert( kv );
   }

#pragma region concurrent_map_test

   auto tic_start = TRI_microtime();

   std::vector<std::thread> threads;
   for ( int i = 0; i < reader_count; ++i )
   {
      threads.push_back( std::thread( ReadFunc, std::ref( concurrent_map ), iter_count ) );
   }

   for ( int i = 0; i < updater_count; ++i )
   {
      threads.push_back( std::thread( UpdateFunc, std::ref( concurrent_map ), iter_count ) );
   }

   for ( int i = 0; i < inserter_count; ++i )
   {
      threads.push_back( std::thread( InsertFunc, std::ref( concurrent_map ), iter_count ) );
   }

   for ( int i = 0; i < num_threads; ++i )
   {
      threads[i].join();
   }

   std::cout
      << "Container: ThreadsafeHashTable"
      << " Threads: "
      << num_threads
      <<" Iterations: "
      << iter_count
      << " Readers: "
      << reader_count
      << " Updaters: "
      << updater_count
      << " Inserters: "
      << inserter_count
      << " Duration: "
      << ( float ) ( TRI_microtime() - tic_start )
      << " Container size: "
      << concurrent_map.Size()
      << "\n"
      << "\n";

#pragma endregion concurrent_map_test

#pragma region serial_map_test

   TSerialMap serial_map;
   for ( size_t i = 0; i < initial_size; ++i )
   {
      TKeyValue kv;
      kv.first = distribution( generator );
      kv.second = distribution( generator );

      serial_map.insert( kv );
   }

   tic_start = TRI_microtime();

   for ( size_t i = 0; i < reader_count; ++i )
   {
      ReadSerial( serial_map, iter_count );
   }

   for ( size_t i = 0; i < updater_count; ++i )
   {
      UpdateSerial( serial_map, iter_count );
   }

   for ( size_t i = 0; i < inserter_count; ++i )
   {
      InsertSerial( serial_map, iter_count );
   }

   std::cout
      << "Container: std::map"
      << " Threads: "
      << 1
      << " Reads: "
      << reader_count * iter_count
      << " Updates: "
      << updater_count * iter_count
      << " Inserts: "
      << inserter_count * iter_count
      << " Duration: "
      << ( float ) ( TRI_microtime() - tic_start )
      << " Container size: "
      << serial_map.size()
      << "\n"
      << "\n";

#pragma endregion serial_map_test

}