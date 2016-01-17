#include "SlimReaderWriterLock.h"

namespace kvs
{

SlimReaderWriterLock::SlimReaderWriterLock()
{
   InitializeSRWLock( &mSrwlock );
}

SlimReaderWriterLock::~SlimReaderWriterLock()
{

}

void SlimReaderWriterLock::lock()
{
   AcquireSRWLockExclusive( &mSrwlock );
}

void SlimReaderWriterLock::unlock()
{
   ReleaseSRWLockExclusive( &mSrwlock );
}

void SlimReaderWriterLock::lock_shared()
{
   AcquireSRWLockShared( &mSrwlock );
}

void SlimReaderWriterLock::unlock_shared()
{
   ReleaseSRWLockShared( &mSrwlock );
}

bool SlimReaderWriterLock::try_lock()
{
   return TryAcquireSRWLockExclusive( &mSrwlock ) != 0;
}

bool SlimReaderWriterLock::try_lock_shared()
{
   return TryAcquireSRWLockShared( &mSrwlock ) != 0;
}

} // namespace kvs