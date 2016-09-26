// EPOS Cortex Timer Mediator Declarations

#ifndef __cortex_timer_h
#define __cortex_timer_h

#include <cpu.h>
#include <ic.h>
#include <rtc.h>
#include <timer.h>
#include <machine.h>
#include __MODEL_H

__BEGIN_SYS

#ifdef __mmod_zynq__

// Cortex-A Private Timer
class System_Timer_Engine: public Machine_Model
{
public:
    typedef CPU::Reg32 Count;
    static const unsigned int CLOCK = Traits<CPU>::CLOCK/2;

protected:
    System_Timer_Engine() {}

public:
    static TSC::Hertz clock() { return CLOCK; }

    static void enable() { priv_timer(PTCLR) |= TIMER_ENABLE; }
    static void disable() { priv_timer(PTCLR) &= ~TIMER_ENABLE; }

    static void isr_clr() { priv_timer(PTISR) = INT_CLR; }

    void power(const Power_Mode & mode);

    static void init(unsigned int f) {
        priv_timer(PTCLR) = 0;
        isr_clr();
        priv_timer(PTLR) = CLOCK / f;
        priv_timer(PTCLR) = IRQ_EN | AUTO_RELOAD;
    }
};

// Cortex-A Global Timer
class User_Timer_Engine: public Machine_Model
{
private:
    typedef CPU::Reg64 Count;
    typedef TSC::Hertz Hertz;

public:
    static const Hertz CLOCK = Traits<CPU>::CLOCK/2;

public:
    User_Timer_Engine(unsigned int channel, const Count & count, bool interrupt = true, bool periodic = true);

    static Hertz clock() { return CLOCK; }

    Count read() {
        Reg32 high, low;

        do {
            high = global_timer(GTCTRH);
            low = global_timer(GTCTRL);
        } while(global_timer(GTCTRH) != high);

        return static_cast<Count>(high) << 32 | low;
    }

    static void enable();
    static void disable();

    void set(const Count & count) {
        // Disable counting before programming
        global_timer(GTCLR) = 0;

        global_timer(GTCTRL) = count & 0xffffffff;
        global_timer(GTCTRH) = count >> 32;

        // Re-enable counting
        global_timer(GTCLR) = 1;
    }
};

#else

// Cortex-M SysTick Timer
class System_Timer_Engine: public Machine_Model
{
public:
    typedef CPU::Reg32 Count;
    static const TSC::Hertz CLOCK = Traits<CPU>::CLOCK;

protected:
    System_Timer_Engine() {}

public:
    static TSC::Hertz clock() { return CLOCK; }

    static void enable() { scs(STCTRL) |= ENABLE; }
    static void disable() { scs(STCTRL) &= ~ENABLE; }

    static void isr_clr() {}

    static void init(unsigned int f) {
        scs(STCTRL) = 0;
        scs(STCURRENT) = 0;
        scs(STRELOAD) = CLOCK / f;
        scs(STCTRL) = CLKSRC | INTEN;
    }
};

// Cortex-M General Purpose Timer
class User_Timer_Engine: public Machine_Model
{
protected:
    const static unsigned int CLOCK = Traits<CPU>::CLOCK;

    typedef CPU::Reg32 Count;

protected:
    User_Timer_Engine(unsigned int channel, const Count & count, bool interrupt = true, bool periodic = true)
    : _channel(channel), _base(reinterpret_cast<Reg32 *>(TIMER0_BASE + 0x1000 * channel)) {
        disable();
        power_user_timer(channel, FULL);
        reg(GPTMCFG) = 0; // 32-bit timer
        reg(GPTMTAMR) = periodic ? 2 : 1; // 2 -> Periodic, 1 -> One-shot
        reg(GPTMTAILR) = count;
        enable();
    }

public:
    ~User_Timer_Engine() { disable(); power_user_timer(_channel, OFF); }

    unsigned int clock() const { return CLOCK; }

    Count read() { return reg(GPTMTAR); }

    void enable() { reg(GPTMCTL) |= TAEN; }
    void disable() { reg(GPTMCTL) &= ~TAEN; }

    void pwm(const Percent & duty_cycle) {
        disable();
        Count count = reg(GPTMTAILR);
        reg(GPTMCFG) = 4; // 4 -> 16-bit, only possible value for PWM
        reg(GPTMTAMR) = TCMR | TAMS | 2; // 2 -> Periodic, 1 -> One-shot
        reg(GPTMTAPR) = count >> 16;
        reg(GPTMTAMATCHR) = percent2count(duty_cycle, count);
        reg(GPTMTAPMR) = reg(GPTMTAMATCHR) >> 16;
        reg(GPTMCTL) &= ~TBPWML; // never inverted
        enable();
    }

private:
    volatile Reg32 & reg(unsigned int o) { return _base[o / sizeof(Reg32)]; }

    static Count percent2count(const Percent & duty_cycle, const Count & period) {
        return period - ((period * duty_cycle) / 100);
    }

private:
    unsigned int _channel;
    Reg32 * _base;
};

#endif

// Tick timer used by the system
class Timer: private Timer_Common, private System_Timer_Engine
{
    friend class Machine;
    friend class Init_System;

protected:
    static const unsigned int CHANNELS = 2;
    static const unsigned int FREQUENCY = Traits<Timer>::FREQUENCY;

    typedef System_Timer_Engine Engine;
    typedef Engine::Count Count;
    typedef IC::Interrupt_Id Interrupt_Id;

public:
    using Timer_Common::Hertz;
    using Timer_Common::Tick;
    using Timer_Common::Handler;

    // Channels
    enum {
        SCHEDULER,
        ALARM,
        USER
    };

protected:
    Timer(unsigned int channel, const Hertz & frequency, const Handler & handler, bool retrigger = true)
    : _channel(channel), _initial(FREQUENCY / frequency), _retrigger(retrigger), _handler(handler) {
        db<Timer>(TRC) << "Timer(f=" << frequency << ",h=" << reinterpret_cast<void*>(handler) << ",ch=" << channel << ") => {count=" << _initial << "}" << endl;

        if(_initial && (channel < CHANNELS) && !_channels[channel])
            _channels[channel] = this;
        else
            db<Timer>(WRN) << "Timer not installed!"<< endl;

        for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++)
            _current[i] = _initial;
    }

public:
    ~Timer() {
        db<Timer>(TRC) << "~Timer(f=" << frequency() << ",h=" << reinterpret_cast<void*>(_handler) << ",ch=" << _channel << ") => {count=" << _initial << "}" << endl;

        _channels[_channel] = 0;
    }

    Hertz frequency() const { return (FREQUENCY / _initial); }
    void frequency(const Hertz & f) { _initial = FREQUENCY / f; reset(); }

    Tick read() { return _current[Machine::cpu_id()]; }

    int reset() {
        db<Timer>(TRC) << "Timer::reset() => {f=" << frequency()
                       << ",h=" << reinterpret_cast<void*>(_handler)
                       << ",count=" << _current[Machine::cpu_id()] << "}" << endl;

        int percentage = _current[Machine::cpu_id()] * 100 / _initial;
        _current[Machine::cpu_id()] = _initial;

        return percentage;
    }

    void handler(const Handler & handler) { _handler = handler; }

    static void isr_clr() { Engine::isr_clr(); }

private:
    static Hertz count2freq(const Count & c) { return c ? Engine::clock() / c : 0; }
    static Count freq2count(const Hertz & f) { return f ? Engine::clock() / f : 0;}

    static void int_handler(const Interrupt_Id & i);

    static void init();

private:
    unsigned int _channel;
    Count _initial;
    bool _retrigger;
    volatile Count _current[Traits<Machine>::CPUS];
    Handler _handler;

    static Timer * _channels[CHANNELS];
};

// Timer used by Thread::Scheduler
class Scheduler_Timer: public Timer
{
private:
    typedef RTC::Microsecond Microsecond;

public:
    Scheduler_Timer(const Microsecond & quantum, const Handler & handler): Timer(SCHEDULER, 1000000 / quantum, handler) {}
};

// Timer used by Alarm
class Alarm_Timer: public Timer
{
public:
    static const unsigned int FREQUENCY = Timer::FREQUENCY;

public:
    Alarm_Timer(const Handler & handler): Timer(ALARM, FREQUENCY, handler) {}
};


// User timer
class User_Timer: private Timer_Common, private User_Timer_Engine
{
    friend class PWM;

private:
    typedef User_Timer_Engine Engine;

public:
    using Timer_Common::Microsecond;
    using Timer_Common::Handler;

public:
    User_Timer(unsigned int channel, const Microsecond & time, const Handler & handler, bool periodic = false)
    : Engine(channel, us2count(time), handler ? true : false, periodic), _channel(channel), _handler(handler) {}
    ~User_Timer() {}

    Microsecond read() { return count2us(Engine::read()); }

    void enable() { Engine::enable(); }
    void disable() { Engine::disable(); }
    void power(const Power_Mode & mode) { power_user_timer(_channel, mode); }

private:
    static void int_handler(const IC::Interrupt_Id & i);

    static Reg32 us2count(const Microsecond & us) { return us * (CLOCK / 1000000); }
    static Microsecond count2us(Reg32 count) { return count / (CLOCK / 1000000); }

private:
    unsigned int _channel;
    Handler _handler;
};

__END_SYS

#endif
