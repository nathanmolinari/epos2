// EPOS IA32 Architecture Metainfo

#ifndef __ia32_traits_h
#define __ia32_traits_h

#include <system/config.h>

__BEGIN_SYS

template<> struct Traits<IA32>: public Traits<void>
{
    enum {LITTLE, BIG};
    static const unsigned int ENDIANESS         = LITTLE;
    static const unsigned int WORD_SIZE         = 32;
    static const unsigned int CLOCK             = 2000000000;
    static const bool unaligned_memory_access   = true;
};

template<> struct Traits<IA32_TSC>: public Traits<void>
{
};

template<> struct Traits<IA32_MMU>: public Traits<void>
{
    static const bool colorful = false;
    static const unsigned int COLORS = 1;
};

template<> struct Traits<IA32_FPU>: public Traits<void>
{
    static const bool enabled = false;
};

template<> struct Traits<IA32_PMU>: public Traits<void>
{
    static const bool enabled = true;
    enum { V1, V2, V3, DUO, MICRO, ATOM, NEHALEN, NETBURST, SANDY_BRIDGE };
    static const unsigned int VERSION = V2;
};

__END_SYS

#endif
