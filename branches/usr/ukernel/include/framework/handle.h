// EPOS Component Framework - Component Handle

// Handle is the framework entry point. It defines a first component wrapper whose main
// purpose is to ensure the invocation of proper new and delete operators (from scenario) for components,
// independently of how they are declared by the client (static allocations will be forwarded to new).

#ifndef __handle_h
#define __handle_h

#include "stub.h"

__BEGIN_SYS

template<typename Component>
class Handle;

// Handled is used to create a wrapper for components created either by SETUP before the framework came into place or internally by the framework itself
template<typename Component>
class Handled: public Handle<Component>
{
public:
    Handled(): Handle<Component>(Handle<Component>::HANDLED) {
        db<Framework>(TRC) << "Handled(this=" << this << ")" << endl;
    }

    void * operator new(size_t s, void * stub) {
        db<Framework>(TRC) << "Handled::new(stub=" << stub << ")" << endl;
        Framework::Element * el= Framework::_cache.search_key(reinterpret_cast<unsigned int>(stub));
        void * handle;
        if(el) {
            handle = el->object();
            db<Framework>(INF) << "Handled::new(stub=" << stub << ") => " << handle << " (CACHED)" << endl;
        } else {
            handle = new Handle<Component>(reinterpret_cast<typename Handle<Component>::_Stub *>(stub));
            el = new Framework::Element(handle, reinterpret_cast<unsigned int>(stub));  // the handled cache is insert-only; object are intentionally never deleted, since they have been created by SETUP!
            Framework::_cache.insert(el);
        }
        return handle;
    }
};


template<typename Component>
class Handle
{
    template<typename> friend class Handle;
    template<typename> friend class Handled;
    template<typename> friend class Proxy;

private:
    typedef Stub<Component, Traits<Component>::ASPECTS::Length || (Traits<System>::mode == Traits<Build>::KERNEL)> _Stub;

    enum Private_Handle{ HANDLED };

private:
    Handle(const Private_Handle & h) { db<Framework>(TRC) << "Handle(HANDLED) => [stub=" << _stub << "]" << endl; }
    Handle(_Stub * s) { _stub = s; }

public:
    template<typename ... Tn>
    Handle(const Tn & ... an) { _stub = new _Stub(an ...); }

    // Dereferencing handles for Task(cs, ds, ...)
    template<typename ... Tn>
    Handle(Handle<Segment> * cs, Handle<Segment> * ds, const Tn & ... an) { _stub = new _Stub(*cs->_stub, *ds->_stub, an ...); }

    ~Handle() { if(_stub) delete _stub; }

    static Handle<Component> * self() { return new (_Stub::self()) Handled<Component>; }

    // Process management
    int priority() { return _stub->priority(); }
    void priority(int p) { _stub->priority(p); }
    int join() { return _stub->join(); }
    int pass() { return _stub->pass(); }
    void suspend() { _stub->suspend(); }
    void resume() { _stub->resume(); }
    static void yield() { _Stub::yield(); }
    static void exit(int r = 0) { _Stub::exit(r); }
    static volatile bool wait_next() { return _Stub::wait_next(); }

    Handle<Address_Space> * address_space() const { return new (_stub->address_space()) Handled<Address_Space>; }
    Handle<Segment> * code_segment() const { return new (_stub->code_segment()) Handled<Segment>; }
    Handle<Segment> * data_segment() const { return new (_stub->data_segment()) Handled<Segment>; }
    CPU::Log_Addr code() const { return _stub->code(); }
    CPU::Log_Addr data() const { return _stub->data(); }
    Handle<Thread> * main() const { return new (_stub->main()) Handled<Thread>; }
    void main(Handle<Thread> * thread) { _stub->main(thread->_stub); }

    // Memory Management
    CPU::Phy_Addr pd() { return _stub->pd(); }
    CPU::Log_Addr attach(Handle<Segment> * seg) { return _stub->attach(*seg->_stub); }
    CPU::Log_Addr attach(Handle<Segment> * seg, CPU::Log_Addr addr) { return _stub->attach(*seg->_stub, addr); }
    void detach(Handle<Segment> * seg) { _stub->detach(*seg->_stub); }

    unsigned int size() const { return _stub->size(); }
    CPU::Phy_Addr phy_address() const { return _stub->phy_address(); }
    int resize(int amount) { return _stub->resize(amount); }

    static unsigned long physical_address(unsigned long log_addr, unsigned long * out_page_frame_present) { return _Stub::physical_address(log_addr, out_page_frame_present); }
    static void dump_memory_mapping() { _Stub::dump_memory_mapping(); }
    static void check_memory_mapping() { _Stub::check_memory_mapping(); }
    static void set_as_read_only(unsigned long log_addr, unsigned long size, bool user = 1) { _Stub::set_as_read_only(log_addr, size, user); }

    // Synchronization
    void lock() { _stub->lock(); }
    void unlock() { _stub->unlock(); }

    void p() { _stub->p(); }
    void v() { _stub->v(); }

    void wait() { _stub->wait(); }
    void signal() { _stub->signal(); }
    void broadcast() { _stub->broadcast(); }

    // Timing
    template<typename T>
    static void delay(T t) { _Stub::delay(t); }

    void reset() { _stub->reset(); }
    void start() { _stub->start(); }
    void lap() { _stub->lap(); }
    void stop() { _stub->stop(); }

    int frequency() { return _stub->frequency(); }
    int ticks() { return _stub->ticks(); }
    int read() { return _stub->read(); }

    static TSC::Time_Stamp time_stamp() { return _Stub::time_stamp(); }

    static Chronometer_Aux::Nanosecond elapsed_nano(TSC::Time_Stamp start, TSC::Time_Stamp stop)
    {
        return _Stub::elapsed_nano(start, stop);
    }

    static Chronometer_Aux::Microsecond elapsed_micro(TSC::Time_Stamp start, TSC::Time_Stamp stop)
    {
        return _Stub::elapsed_micro(start, stop);
    }

    static Chronometer_Aux::Second elapsed_sec(TSC::Time_Stamp start, TSC::Time_Stamp stop)
    {
        return _Stub::elapsed_sec(start, stop);
    }

    static Chronometer_Aux::Nanosecond nano(TSC::Time_Stamp ticks)
    {
        return _Stub::nano(ticks);
    }

    static Chronometer_Aux::Microsecond micro(TSC::Time_Stamp ticks)
    {
        return _Stub::micro(ticks);
    }

    static Chronometer_Aux::Second sec(TSC::Time_Stamp ticks)
    {
        return _Stub::sec(ticks);
    }


    // Communication
    template<typename ... Tn>
    int send(Tn ... an) { return _stub->send(an ...); }
    template<typename ... Tn>
    int receive(Tn ... an) { return _stub->receive(an ...); }

    template<typename ... Tn>
    int read(Tn ... an) { return _stub->read(an ...); }
    template<typename ... Tn>
    int write(Tn ... an) { return _stub->write(an ...); }

    int tcp_link_read(void * data, unsigned int size) { return _stub->tcp_link_read(data, size); }

    int ether_channel_link_read(void * data, unsigned int size) { return _stub->ether_channel_link_read(data, size); }

    // Network
    static void init_network() { _Stub::init_network(); }

    // NIC
    Handle<NIC::Statistics> * statistics() { return new (_stub->statistics()) Handled<NIC::Statistics>; }
    Handle<NIC::Address> * nic_address() { return (new (_stub->address()) Handled<NIC::Address>); }
    unsigned int nic_mtu() { return _stub->nic_mtu(); }
    int nic_receive(NIC::Address * src, NIC::Protocol * prot, void * data, unsigned int size) { return _stub->nic_receive(src, prot, data, size); }

    // IP
    Handle<NIC> * nic() { return new (_stub->nic()) Handled<NIC>; }
    Handle<IP::Address> * address() { return (new (_stub->address()) Handled<IP::Address>); }
    static Handle<IP> * get_by_nic(unsigned int unit) { return new (_Stub::get_by_nic(unit)) Handled<IP>; }

    // Machine
    static void smp_barrier() { _Stub::smp_barrier(); }
    static unsigned int cpu_id() { return _Stub::cpu_id(); }

    // This_Thread
    static unsigned int this_thread_id() { return _Stub::this_thread_id(); }

    // CPU
    static void int_enable() { _Stub::int_enable(); }
    static void int_disable() { _Stub::int_disable(); }

    // FPGA
    static void run() { _Stub::run(); }
    static void report() { _Stub::report(); }
    static void print_configuration() { _Stub::print_configuration(); }
    static void monitor_start() { _Stub::monitor_start(); }
    static void monitor_stop() { _Stub::monitor_stop(); }

    // UART
    void rts_down() { _stub->rts_down(); }
    void rts_up() { _stub->rts_up(); }

public:
    _Stub * __stub() { return _stub; } // To be used by Subclasses of Handle<> only

private:
    _Stub * _stub;
};

__END_SYS

#endif
