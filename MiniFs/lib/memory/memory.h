#pragma once

typedef unsigned long long size_t;

#define KERNEL
#ifdef KERNEL

#include <Ntddk.h>
#include <Ntifs.h>

#else

#include <stdlib.h>
#include <format>
#include <iostream>

#endif

#define max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

void MemCopy(void* dst, void* src, size_t len);

template <class T>
void ZeroMemory(T* dst, size_t len);

namespace krnl_std
{
    void* Alloc(size_t n);

    void Free(void* p);
}