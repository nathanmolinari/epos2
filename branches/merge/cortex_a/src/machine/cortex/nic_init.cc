// EPOS Cortex NIC Mediator Initialization

#include <machine/cortex/nic.h>

__BEGIN_SYS

template<int unit>
inline static void call_init()
{
    typedef typename Traits<NIC>::NICS::template Get<unit>::Result NIC;

    // TODO: unit should be reset for each different NIC
    if(Traits<NIC>::enabled)
        NIC::init(unit);

    call_init<unit + 1>();
};

template<>
inline void call_init<Traits<NIC>::NICS::Length>()
{
};

void NIC::init()
{
    call_init<0>();
}

__END_SYS
