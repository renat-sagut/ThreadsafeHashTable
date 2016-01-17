# ThreadsafeHashTable
���������� ����������������� ��������� ��� ����-��������.
- ������ �������� � ���-�������. 
- �������� ������ ���������� �������� �� ������������� �������.
- ������ ���������� ����������� ��� ������� �� ������� ������ ������ �� ���������� ����������.
- ������������� ����������� ���������� ��� ������ � ������������ ���������� ��� ������.
- �������������� ��������
#### ��������� ����������
����������� ������� for_each, find_first_if, erase_if.

��� ���������� ���������� ��� � stl �����, ����� �������� ���������� ����������� ��������� ��������, �� ������� �� ���������. ���� ���� ����� ������� ��������� ����������, �� ������ �� ��� ���������� ������������� ���� �������. ��� ����� ����� ����� ������������ recursive_mutex, �� �.�. recursive_shared_mutex ��� � boost � � stl, �� ��������� ������ ������ ����� ����������� ������ � ����������� ������ �������.

��� ��� �� ���� ����� � ����� "C++ Concurrency in action":

The basic issue with
STL-style iterator support is that the iterator must hold some kind of reference into
the internal data structure of the container. If the container can be modified from
another thread, this reference must somehow remain valid, which essentially
requires that the iterator hold a lock on some part of the structure. Given that the
lifetime of an STL-style iterator is completely outside the control of the container,
this is a bad idea
#### ������������������
- ��������� �� - i7-2600, 3.4GHz, Windows 7 64bit
- ������� �� ������ - 2
- ������� �� ���������� - 2
- ������� �� ������� - 4
- ���������� �������� - 1000 000
- ����� ���������� - 0.85 ��� (SlimReaderWriterLock), 1.22 ��� (boost::shared_mutex)
- ����� ���������� ��� �� �������� � ����� ������ ��� std::map - 2.83 ���

#### �������� ������ �� ����
- http://www.bogotobogo.com/cplusplus/files/CplusplusConcurrencyInAction_PracticalMultithreading.pdf
- http://habrahabr.ru/post/250383/
- http://www.codeproject.com/Articles/43284/Testing-simple-concurrent-containers
- https://www.arangodb.com/2015/02/comparing-atomic-mutex-rwlocks/
