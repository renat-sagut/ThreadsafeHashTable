#pragma once

#include <WinSock2.h>

namespace kvs
{

class SlimReaderWriterLock
{
public:
   SlimReaderWriterLock();

   ~SlimReaderWriterLock();

   void lock();

   void unlock();

   void lock_shared();

   void unlock_shared();

   bool try_lock();

   bool try_lock_shared();

private:
   SRWLOCK mSrwlock;
};

} // namespace kvs