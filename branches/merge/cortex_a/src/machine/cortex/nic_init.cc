// EPOS Cortex NIC Mediator Initialization

#include <machine/cortex/nic.h>

__BEGIN_SYS

//template<int unit>
//inline static void call_init()
//{
//    typedef typename Traits<Ethernet>::NICS::template Get<unit>::Result NIC;
//    static const unsigned int OFFSET = Traits<Ethernet>::NICS::template Find<NIC>::Result;
//
//    if(Traits<NIC>::enabled)
//        NIC::init(unit - OFFSET);
//
//    call_init<unit + 1>();
//};
//
//template<>
//inline void call_init<Traits<Ethernet>::NICS::Length>()
//{
//};
//
//void Ethernet::init()
//{
//    call_init<0>();
//}
//
//__END_SYS

template<typename Type, int unit>
inline static void call_init()
{
    typedef typename Traits<Type>::NICS::template Get<unit>::Result NIC;
    static const unsigned int OFFSET = Traits<Type>::NICS::template Find<NIC>::Result;

    if(Traits<NIC>::enabled)
        NIC::init(unit - OFFSET);

    call_init<Type, unit + 1>();
};

template<>
inline void call_init<NIC, Traits<NIC>::NICS::Length>() {}

void NIC::init()
{
    call_init<NIC, 0>();
}

__END_SYS
