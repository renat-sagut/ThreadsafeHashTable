#include "precomp.h"
#include "ThreadsafeHashTable.h"

BOOST_AUTO_TEST_CASE( TestInterface )
{
   try
   {
      kvs::ThreadsafeHashTable<int, int> ht;
      int val;
      ht.Find( 0, val );
      ht.Size();
      ht.Erase( 1 );
      ht.Insert( std::make_pair( 1, 2 ) );
      ht.Update( std::make_pair( 2, 3 ) );
      ht[1];
      ht.ForEach( []( std::pair<int, int> const& ){} );
      ht.Clear();
   }
   catch( ... )
   {
      BOOST_ERROR( "Ouch..." );
   }
}

BOOST_AUTO_TEST_CASE( TestSize )
{
   try
   {
      kvs::ThreadsafeHashTable<int, int> ht;

      ht.Erase( 1 );
      BOOST_CHECK_EQUAL( ht.Size(), 0 );

      ht.Insert( std::make_pair( 1, 2 ) );
      BOOST_CHECK_EQUAL( ht.Size(), 1 );

      ht.Insert( std::make_pair( 1, 3 ) );
      BOOST_CHECK_EQUAL( ht.Size(), 1 );

      ht.Insert( std::make_pair( 2, 2 ) );
      BOOST_CHECK_EQUAL( ht.Size(), 2 );

      ht.Update( std::make_pair( 2, 3 ) );
      BOOST_CHECK_EQUAL( ht.Size(), 2 );

      ht.Erase( 2 );
      BOOST_CHECK_EQUAL( ht.Size(), 1 );

      ht.Clear();
      BOOST_CHECK_EQUAL( ht.Size(), 0 );
   }
   catch( ... )
   {
      BOOST_ERROR( "Ouch..." );
   }
}

BOOST_AUTO_TEST_CASE( TestIterator )
{
   try
   {
      kvs::ThreadsafeHashTable<int, int> ht;
      int size = 100;
      for ( int i = 0; i < size; ++i )
      {
         ht.Insert( std::make_pair( i, i * 2 ) );
      }

      int counter = 0;
      ht.ForEach( [&counter]( std::pair<int, int> const& kv ){ BOOST_CHECK_EQUAL( kv.first * 2, kv.second ); ++counter; } );

      BOOST_CHECK_EQUAL( counter, size );
   }
   catch( ... )
   {
      BOOST_ERROR( "Ouch..." );
   }
}

BOOST_AUTO_TEST_CASE( TestFindFirstIf )
{
   try
   {
      kvs::ThreadsafeHashTable<int, int> ht;
      int size = 100;
      for ( int i = 0; i < size; ++i )
      {
         ht.Insert( std::make_pair( i, i * 2 ) );
      }

      std::pair<int, int> found( 0, 0 );
      ht.FindFirstIf( []( std::pair<int, int> const& kv ){ if ( kv.first == 50 && kv.second == 100 ) return true; return false; }, found );

      BOOST_CHECK_EQUAL( found.first, 50 );
      BOOST_CHECK_EQUAL( found.second, 100 );
   }
   catch( ... )
   {
      BOOST_ERROR( "Ouch..." );
   }
}

BOOST_AUTO_TEST_CASE( TestEraseIf )
{
   try
   {
      kvs::ThreadsafeHashTable<int, int> ht;
      int size = 100;
      for ( int i = 0; i < size; ++i )
      {
         ht.Reserve( i );
         ht.Insert( std::make_pair( i, i * 2 ) );
      }

      ht.EraseIf( []( std::pair<int, int> const& kv ){ if ( kv.first == 50 && kv.second == 100 ) return true; return false; } );

      int val;
      auto exists = ht.Find( 50, val );
      BOOST_CHECK_EQUAL( exists, false );

      exists = ht.Find( 51, val );
      BOOST_CHECK_EQUAL( exists, true );
   }
   catch( ... )
   {
      BOOST_ERROR( "Ouch..." );
   }
}