﻿# ThreadsafeHashTable
Реализация потокобезопасного хранилища пар ключ-значение.
- Данные хранятся в хэш-таблице. 
- Конечный массив блокировок отвечает за синхронизацию потоков.
- Индекс блокировки вычисляется как остаток от деления хэша ключа на количество блокировок.
- Используются разделяемые блокировки для чтения и эксклюзивные блокировки для записи.
- Поддерживается рехэшинг

#### Поддержка итераторов
Реализованы функции for_each, find_first_if, erase_if.

Для реализации итераторов как в stl нужно, чтобы итератор блокировал возможность изменения элемента, на который он ссылается. Если один поток создаст несколько итераторов, то каждый из них попытается заблокировать свой элемент. Для этого нужно будет использовать recursive_mutex, но т.к. recursive_shared_mutex нет в boost и в stl, то итераторы одного потока будут блокировать работу с контейнером другим потокам.

Вот что об этом пишут в книге "C++ Concurrency in action":

The basic issue with
STL-style iterator support is that the iterator must hold some kind of reference into
the internal data structure of the container. If the container can be modified from
another thread, this reference must somehow remain valid, which essentially
requires that the iterator hold a lock on some part of the structure. Given that the
lifetime of an STL-style iterator is completely outside the control of the container,
this is a bad idea
#### Производительность
- Параметры ПК - i7-2600, 3.4GHz, Windows 7 64bit
- Потоков на чтение - 2
- Потоков на обновление - 2
- Потоков на вставку - 4
- Количество итераций - 1000 000
- Время выполнения, сек:

```text
+-------------------+----------------------+---------------------+------------+
| Кол-во блокировок | SlimReaderWriterLock | boost::shared_mutex | std::mutex |
+-------------------+----------------------+---------------------+------------+
|                11 | 0.85                 | 1.22                | 4.86       |
|               256 | 0.49                 | 0.58                | 0.70       |
+-------------------+----------------------+---------------------+------------+
```
- Время выполнения тех же операций в одном потоке для std::map - 5.07 сек

#### Полезные ссылки по теме
- http://www.bogotobogo.com/cplusplus/files/CplusplusConcurrencyInAction_PracticalMultithreading.pdf
- http://habrahabr.ru/post/250383/
- http://www.codeproject.com/Articles/43284/Testing-simple-concurrent-containers
- https://www.arangodb.com/2015/02/comparing-atomic-mutex-rwlocks/
