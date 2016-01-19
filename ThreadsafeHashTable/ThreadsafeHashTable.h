#pragma once

#include "precomp.h"

namespace kvs
{

template <typename TKey, typename TValue, size_t pLockCount = 11, typename TLock = boost::shared_mutex, typename THash = std::hash<TKey>>
class ThreadsafeHashTable
{
public:
   typedef std::pair<TKey, TValue> TKeyValue;
   typedef boost::shared_lock_guard<TLock> TSharedLockGuard;
   //typedef std::unique_lock<TLock> TSharedLockGuard;
   typedef std::unique_lock<TLock> TUniqueLockGuard;
   typedef std::array<TLock, pLockCount> TLockContainer;

   class Bucket;
   typedef std::vector<Bucket> TBucketContainer;
   typedef typename TBucketContainer::iterator TBucketIterator;
   typedef std::list<TKeyValue> TCollisionContainer;
   typedef typename TCollisionContainer::iterator TCollisionIterator;
   typedef std::atomic<size_t> TAtomicSize;

   class Bucket
   {
   public:
      Bucket()
      {

      }

      Bucket( Bucket const& b )
      {
         mValues = b.mValues;
      }

      ~Bucket()
      {

      }

      Bucket& operator=( Bucket const& b )
      {
         if ( this == &b )
            return *this;

         mValues = b.mValues;
         return *this;
      }

      TCollisionContainer const& Values()
      {
         return mValues;
      }

      void Clear()
      {
         mValues.clear();
      }

      bool Insert( TKey const& key, TValue const& value )
      {
         auto insert_position = mValues.begin();
         for ( ; insert_position != mValues.end(); ++insert_position )
         {
            auto const& cur_bval = *insert_position;
            auto const& cur_key = cur_bval.first;

            if ( cur_key < key )
            {
               continue;
            }
            else if ( cur_key == key )
            {
               return false;
            }
            else
            {
               break;
            }
         }

         mValues.insert( insert_position, TKeyValue( key, value ) );
         return true;
      }

      bool Update( TKey const& key, TValue const& value )
      {
         for ( auto bval_it = mValues.begin(); bval_it != mValues.end(); ++bval_it )
         {
            auto& cur_bval = *bval_it;
            auto const& cur_key = cur_bval.first;

            if ( cur_key == key )
            {
               cur_bval.second = value;
               return true;
            }
         }

         return false;
      }

      bool Read( TKey const& key, TValue& value ) const
      {
         for ( auto val_it = mValues.begin(); val_it != mValues.end(); ++val_it )
         {
            auto& cur_val = *val_it;
            auto const& cur_key = cur_val.first;

            if ( cur_key > key )
            {
               return false;
            }
            else if ( cur_key == key )
            {
               value = cur_val.second;
               return true;
            }
         }

         return false;
      }

      TCollisionIterator Find( TKey const& key )
      {
         for ( auto val_it = mValues.begin(); val_it != mValues.end(); ++val_it )
         {
            auto& cur_val = *val_it;
            auto const& cur_key = cur_val.first;

            if ( cur_key > key )
            {
               return mValues.end();
            }
            else if ( cur_key == key )
            {
               return val_it;
            }
         }

         return mValues.end();
      }

      bool Delete( TKey const& key )
      {
         for ( auto bval_it = mValues.begin(); bval_it != mValues.end(); ++bval_it )
         {
            auto& cur_bval = *bval_it;
            auto const& cur_key = cur_bval.first;

            if ( cur_key > key )
            {
               return false;
            }
            else if ( cur_key == key )
            {
               mValues.erase( bval_it );
               return true;
            }
         }

         return false;
      }

      size_t Size() const
      {
         return mValues.size();
      }

      template<typename Function>
      void ForEach( Function f )
      {
         for ( auto val_it = mValues.begin(), end_it = mValues.end(); val_it != end_it; ++val_it )
         {
            f( *val_it );
         }
      }

      template<typename Predicate>
      bool FindFirstIf( Predicate p, TKeyValue& found )
      {
         for ( auto val_it = mValues.begin(), end_it = mValues.end(); val_it != end_it; ++val_it )
         {
            auto const& val = *val_it;

            if( p( val ) )
            {
               found = val;
               return true;
            }
         }

         return false;
      }

      template<typename Predicate>
      bool EraseIf( Predicate p )
      {
         for ( auto val_it = mValues.begin(), end_it = mValues.end(); val_it != end_it; ++val_it )
         {
            auto const& val = *val_it;

            if( p( val ) )
            {
               mValues.erase( val_it );
               return true;
            }
         }

         return false;
      }

   private:
      TCollisionContainer mValues;
   };

   ThreadsafeHashTable()
      : mMaxLoadFactor( 0.7f )
   {
      mBuckets.resize( pLockCount );
   }

   ~ThreadsafeHashTable()
   {

   }

   bool Find( TKey const& key, TValue& value )
   {
      return Read( key, value );
   }

   size_t Size() const
   {
      return mSize;
   }

   // пользователю этого метода придется блокировать ячейку в таблице
   //TValue& operator[]( TKey const& key );

   TValue operator[]( TKey const& key )
   {
      TValue value;
      if ( Read( key, value ) )
         return value;
      throw std::exception( "Key not found" );
   }

   bool Insert( TKeyValue const& kv )
   {
      auto const& key = kv.first;
      auto const& value = kv.second;

      return Insert( key, value );
   }

   bool Update( TKeyValue const& kv )
   {
      auto const& key = kv.first;
      auto const& value = kv.second;

      return Update( key, value );
   }

   void Erase( TKey const& key )
   {
      Delete( key );
   }

   void Clear()
   {
      for ( auto lock_it = mLocks.begin(); lock_it != mLocks.end(); ++lock_it )
      {
         lock_it->lock();
      }

      for ( auto buc_it = mBuckets.begin(); buc_it != mBuckets.end(); ++buc_it )
      {
         buc_it->Clear();
      }

      mSize = 0;

      for ( auto lock_it = mLocks.rbegin(); lock_it != mLocks.rend(); ++lock_it )
      {
         lock_it->unlock();
      }
   }

   template<typename Function>
   void ForEach( Function f )
   {
      TSharedLockGuard rehashLock( mRehashLock );
      for ( auto buc_it = mBuckets.begin(), end_it = mBuckets.end(); buc_it != end_it; ++buc_it )
      {
         auto const idx = std::distance( mBuckets.begin(), buc_it );
         TUniqueLockGuard bucketLock( GetLockForBucket( idx ) );
         buc_it->ForEach( f );
      }
   }

   template<typename Predicate>
   bool FindFirstIf( Predicate p, TKeyValue& found )
   {
      TSharedLockGuard rehashLock( mRehashLock );
      for ( auto buc_it = mBuckets.begin(), end_it = mBuckets.end(); buc_it != end_it; ++buc_it )
      {
         auto const idx = std::distance( mBuckets.begin(), buc_it );
         TSharedLockGuard bucketLock( GetLockForBucket( idx ) );
         if ( buc_it->FindFirstIf( p, found ) )
         {
            return true;
         }
      }

      return false;
   }

   template<typename Predicate>
   bool EraseIf( Predicate p )
   {
      TSharedLockGuard rehashLock( mRehashLock );
      for ( auto buc_it = mBuckets.begin(), end_it = mBuckets.end(); buc_it != end_it; ++buc_it )
      {
         auto const idx = std::distance( mBuckets.begin(), buc_it );
         TUniqueLockGuard bucketLock( GetLockForBucket( idx ) );
         if ( buc_it->EraseIf( p ) )
         {
            return true;
         }
      }

      return false;
   }

   void Reserve( size_t const bucketCount )
   {
      if ( bucketCount <= mBuckets.size() )
         return;

      TUniqueLockGuard rehashLock( mRehashLock );
      Rehash( bucketCount );
   }

private:
   ThreadsafeHashTable( ThreadsafeHashTable const& );

   ThreadsafeHashTable& operator=( ThreadsafeHashTable const& );

   bool NeedRehash()
   {
      auto const loadFactor = static_cast<float>( mSize ) / mBuckets.size();
      if ( loadFactor < mMaxLoadFactor )
         return false;
      return true;
   }

   bool TryRehash()
   {
      if( !NeedRehash() )
         return false;

      // чтобы не запускать рехэш из разных потоков
      if ( !mRehashLock.try_lock() )
         return false;
      TUniqueLockGuard rehash_lock( mRehashLock, std::adopt_lock );

      Rehash();
      return true;
   }

   void Rehash( size_t const size = 0 )
   {
      std::vector<std::shared_ptr<TUniqueLockGuard>> lockGuards;
      for ( auto lock_it = mLocks.begin(); lock_it != mLocks.end(); ++lock_it )
      {
         lockGuards.push_back( std::make_shared<TUniqueLockGuard>( *lock_it ) );
      }
      
      size_t bucketCount;
      if ( size == 0 )
         bucketCount = mBuckets.size() * 2;
      else
         bucketCount = size;

      TBucketContainer oldBuckets = std::move( mBuckets );
      mBuckets.resize( bucketCount );
      for ( auto buc_it = oldBuckets.begin(); buc_it != oldBuckets.end(); ++buc_it )
      {
         auto const& values = buc_it->Values();
         for ( auto val_it = values.begin(); val_it != values.end(); ++val_it )
         {
            auto const& key = val_it->first;
            auto const& value = val_it->second;

            GetBucket( key ).Insert( key, value );
         }
      }
   }

   TLock& GetLockForBucket( size_t const buc_idx )
   {
      auto const idx = buc_idx % mLocks.size();
      return mLocks[idx];
   }

   TLock& GetLockForKey( TKey const& key )
   {
      return GetLockForBucket( GetBucketIndex( key ) );
   }

   size_t GetBucketIndex( TKey const& key )
   {
      return mHasher( key ) % mBuckets.size();
   }

   Bucket& GetBucket( TKey const& key )
   {
      return mBuckets[GetBucketIndex( key )];
   }

   bool Insert( TKey const& key, TValue const& value )
   {
      bool res = false;
      {
         TUniqueLockGuard lock( GetLockForKey( key ) );
         res = GetBucket( key ).Insert( key, value );
      }

      if( res )
      {
         ++mSize;
         TryRehash();
      }

      return res;
   }

   bool Update( TKey const& key, TValue const& value )
   {
      TUniqueLockGuard lock( GetLockForKey( key ) );
      return GetBucket( key ).Update( key, value );
   }

   bool Read( TKey const& key, TValue& value )
   {
      TSharedLockGuard lock( GetLockForKey( key ) );
      return GetBucket( key ).Read( key, value );
   }

   bool Delete( TKey const& key )
   {
      TUniqueLockGuard lock( GetLockForKey( key ) );
      auto res = GetBucket( key ).Delete( key );
      if( res )
         --mSize;
      return res;
   }

   THash mHasher;

   TBucketContainer mBuckets;

   TAtomicSize mSize;

   mutable TLockContainer mLocks;

   mutable TLock mRehashLock;

   float const mMaxLoadFactor;
};

} // namespace kvs