// EPOS ARMv7 CPU Mediator Initialization

#include <cpu.h>
#include <tsc.h>
#include <mmu.h>
#include <system.h>
#include <system/info.h>

extern "C" { void __epos_library_app_entry(void); }

__BEGIN_SYS

void CPU::init()
{
    db<Init, CPU>(TRC) << "CPU::init()" << endl;

    if(Traits<MMU>::enabled)
        MMU::init();
    else
        db<Init, MMU>(WRN) << "MMU is disabled!" << endl;

#ifdef __mmod_zynq__
    if(Traits<TSC>::enabled)
        TSC::init();
#endif
}

__END_SYS
