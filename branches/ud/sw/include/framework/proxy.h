// EPOS Component Framework - Component Proxy

// Proxies and Agents handle RMI within EPOS component framework

#ifndef __proxy_h
#define __proxy_h

#include "message.h"

__BEGIN_SYS

template<typename Component>
class Proxy;

// Proxied is used to create a Proxy for components created either by SETUP before the framework came into place or internally by the framework itself
template<typename Component>
class Proxied: public Proxy<Component>
{
public:
    Proxied(): Proxy<Component>(Proxy<Component>::PROXIED) {
        db<Framework>(TRC) << "Proxied(this=" << this << ")" << endl;
    }

    void * operator new(size_t s, void * adapter) {
        db<Framework>(TRC) << "Proxied::new(adapter=" << adapter << ")" << endl;
        Framework::Element * el= Framework::_cache.search_key(reinterpret_cast<unsigned int>(adapter));
        void * proxy;
        if(el) {
            proxy = el->object();
            db<Framework>(INF) << "Proxied::new(adapter=" << adapter << ") => " << proxy << " (CACHED)" << endl;
        } else {
            proxy = new Proxy<Component>(Id(Type<Component>::ID, reinterpret_cast<Id::Unit_Id>(adapter)));
            el = new Framework::Element(proxy, reinterpret_cast<unsigned int>(adapter));  // the proxied cache is insert-only; object are intentionally never deleted, since they have been created by SETUP!
            Framework::_cache.insert(el);
        }
        return proxy;
    }
};

template<typename Component>
class Proxy: public IF<Traits<Component>::Hardware, Message_UD, Message_Kernel>::Result
{
    typedef IF<Traits<Component>::Hardware, Message_UD, Message_Kernel>::Result Base;

    template<typename> friend class Proxy;
    template<typename> friend class Proxied;

private:
    enum Private_Proxied{ PROXIED };

private:
    Proxy(const Id & id): Base(id) {} // for Proxied::operator new()
    Proxy(const Private_Proxied & p) { db<Framework>(TRC) << "Proxy(PROXIED) => [id=" << Proxy<Component>::id() << "]" << endl; } // for Proxied

public:
    template<typename ... Tn>
    Proxy(const Tn & ... an): Base(Id(Type<Component>::ID, 0)) { invoke(CREATE + sizeof ... (Tn), an ...); }
    ~Proxy() { invoke(DESTROY); }

    static Proxy<Component> * self() { return new (reinterpret_cast<void *>(static_invoke(SELF))) Proxied<Component>; }

    // Process management
    void suspend() { invoke(THREAD_SUSPEND); }
    void resume() { invoke(THREAD_RESUME); }
    int join() { return invoke(THREAD_JOIN); }
    int pass() { return invoke(THREAD_PASS); }
    static int yield() { return static_invoke(THREAD_YIELD); }
    static void exit(int r) { static_invoke(THREAD_EXIT, r); }
    static volatile bool wait_next() { return static_invoke(THREAD_WAIT_NEXT); }

    Proxy<Address_Space> * address_space() { return new (reinterpret_cast<Adapter<Address_Space> *>(invoke(TASK_ADDRESS_SPACE))) Proxied<Address_Space>; }
    Proxy<Segment> * code_segment() { return new (reinterpret_cast<Adapter<Segment> *>(invoke(TASK_CODE_SEGMENT))) Proxied<Segment>; }
    Proxy<Segment> * data_segment() { return new (reinterpret_cast<Adapter<Segment> *>(invoke(TASK_DATA_SEGMENT))) Proxied<Segment>; }
    CPU::Log_Addr code() { return invoke(TASK_CODE); }
    CPU::Log_Addr data() { return invoke(TASK_DATA); }

    // Memory management
    CPU::Phy_Addr pd() { return invoke(ADDRESS_SPACE_PD); }
    CPU::Log_Addr attach(const Proxy<Segment> & seg) { return invoke(ADDRESS_SPACE_ATTACH1, seg.id().unit()); }
    CPU::Log_Addr attach(const Proxy<Segment> & seg, CPU::Log_Addr addr) { return invoke(ADDRESS_SPACE_ATTACH2, seg.id().unit(), addr); }
    void detach(const Proxy<Segment> & seg) { invoke(ADDRESS_SPACE_DETACH, seg.id().unit());}

    unsigned int size() { return invoke(SEGMENT_SIZE); }
    CPU::Phy_Addr phy_address() { return invoke(SEGMENT_PHY_ADDRESS); }
    int resize(int amount) { return invoke(SEGMENT_RESIZE, amount); }

    // Synchronization
    void lock() { invoke(SYNCHRONIZER_LOCK); }
    void unlock() { invoke(SYNCHRONIZER_UNLOCK); }

    void p() { invoke(SYNCHRONIZER_P); }
    void v() { invoke(SYNCHRONIZER_V); }

    void wait() { invoke(SYNCHRONIZER_WAIT); }
    void signal() { invoke(SYNCHRONIZER_SIGNAL); }
    void broadcast() { invoke(SYNCHRONIZER_BROADCAST); }

    // Timing
    template<typename T>
    static void delay(T t) { static_invoke(ALARM_DELAY, t); }

    // Communication
    template<typename T1, typename T2, typename T3>
    int send(T1 a1, T2 a2, T3 a3) { return invoke(SELF, a1, a2, a3); }
    template<typename T1, typename T2, typename T3>
    int receive(T1 a1, T2 a2, T3 a3) { return invoke(SELF, a1, a2, a3); }

    // Adder
    int add(int a, int b) { return invoke(ADDER_ADD, a, b); }

    template<typename ... Tn>
    static int static_invoke(const Method & m, const Tn & ... an) {
        Base msg(Id(Type<Component>::ID, 0)); // avoid calling ~Proxy()
        Result res = msg.act(m, an ...);
        return (m == SELF) ? msg.id().unit() : res;
    }

private:
    template<typename ... Tn>
    int invoke(const Method & m, const Tn & ... an) { return act(m, an ...); }
};

__END_SYS

#endif
