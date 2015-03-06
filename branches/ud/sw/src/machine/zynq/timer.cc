// EPOS Zynq Timer Mediator Implementation

#include <machine.h>
#include <ic.h>
#include <machine/zynq/timer.h>

__BEGIN_SYS

// Class attributes
Zynq_Timer * Zynq_Timer::_channels[CHANNELS];

// Class methods
void Zynq_Timer::int_handler(const IC::Interrupt_Id & i)
{
    if((!Traits<System>::multicore || (Traits<System>::multicore && (Machine::cpu_id() == 0))) && _channels[ALARM])
        _channels[ALARM]->_handler(i);

    if(_channels[SCHEDULER] && (--_channels[SCHEDULER]->_current[Machine::cpu_id()] <= 0)) {
        _channels[SCHEDULER]->_current[Machine::cpu_id()] = _channels[SCHEDULER]->_initial;
        _channels[SCHEDULER]->_handler(i);
    }
}

__END_SYS
