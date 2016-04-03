// EPOS TSTP Definitions Common Package

#ifndef __tstp_api_h
#define __tstp_api_h

#include <cpu.h>
#include <ieee1451_0.h>
#include <tstp_time.h>
#include <tstp_router.h>
#include <tstp_security.h>

__BEGIN_SYS

class TSTP_API
{
public:
    typedef Traits<TSTP>::MAC MAC;
    typedef Traits<TSTP>::Time_Manager Time_Manager;
    typedef Traits<TSTP>::Router Router;
    typedef Traits<TSTP>::Security Security;

    typedef Router::Local_Address Local_Address;
    typedef Router::Remote_Address Remote_Address;
    typedef Router::Hint Address_Hint;
    typedef Time_Manager::Time Time;
    typedef Security::MAC Sec_MAC;

    typedef IEEE1451_0::UNITS::UNIT_CODE_T Unit;
    typedef unsigned char Error;
    //typedef RTC::Microsecond Microsecond;
    //typedef unsigned int Data;
    //typedef Data (* Sensor_Handler)();
    typedef CPU::Reg16 Message_ID;

    // Must be <= PHY::MTU
    static const unsigned int MTU = 120;

    enum MESSAGE_TYPE {
        INTEREST = 0,
        DATA = 1,
        REPORT = 2,
        // 3 is unused
        BOOTSTRAP_0 = 4,
        BOOTSTRAP_1 = 5,
        BOOTSTRAP_2 = 6,
        BOOTSTRAP_3 = 7,
    };

    enum RESPONSE_MODE {
        SINGLE = 0,
        ALL = 1,
    };

    class Header
    {
    public:
        Header() { }
        Header(const MESSAGE_TYPE & t) : _cfg(t << 5) {}
        Header(const MESSAGE_TYPE & t, const Local_Address & origin, const Time & deadline) : _cfg((t << 5)), _last_hop_address(origin), _last_hop_time(0), _origin_address(origin), _origin_time(0), _deadline(deadline) { }

        MESSAGE_TYPE message_type() const { return static_cast<MESSAGE_TYPE>(_cfg >> 5); }
        void message_type(const MESSAGE_TYPE & t) {
            unsigned char tmp = _cfg & ~(7 << 5);
            _cfg = tmp | ((t & 7) << 5);
        }

        bool time_request() const { return _cfg & (1 << 4); }
        void time_request(bool t) {
            unsigned char tmp = _cfg & ~(1 << 4);
            _cfg = tmp | (t << 4);
        }

        unsigned char spatial_scale() const { return (_cfg & (3 << 2)) >> 2; }
        void spatial_scale(unsigned char s) {
            unsigned char tmp = _cfg & ~(3 << 2);
            _cfg = tmp | ((s & 3) << 2);
        }

        unsigned char temporal_scale() const { return _cfg & 3; }
        void temporal_scale(unsigned char s) {
            unsigned char tmp = _cfg & ~(3);
            _cfg = tmp | (s & 3);
        }

        const Local_Address & last_hop_address() const { return _last_hop_address; }
        void last_hop_address(const Local_Address & _addr) { _last_hop_address = _addr; }

        const Local_Address & origin_address() const { return _origin_address; }
        void origin_address(const Local_Address & _addr) { _origin_address = _addr; }

        Time last_hop_time() const { return _last_hop_time; }
        void last_hop_time(const Time & t) { _last_hop_time = t; }

        Time origin_time() const { return _origin_time; }
        void origin_time(const Time & t) { _origin_time = t; }

        Time deadline() const { return _deadline; }
        void deadline(const Time & t) { _deadline = t; }

        friend Debug & operator<<(Debug & db, const Header & h) {
            db << "{type=" << h.message_type() << ",tr=" << h.time_request() << ",sscale=" << h.spatial_scale() << ",tscale=" << h.temporal_scale() << ",lconf=" << ",lhaddr=" << h._last_hop_address << ",lht=" << h._last_hop_time << ",oaddr=" << h._origin_address << ",ot=" << h._origin_time << ",de=" << h._deadline << "}" << endl;
            return db;
        }
        friend OStream & operator<<(OStream & os, const Header & h) {
            os << "{type=" << h.message_type() << ",tr=" << h.time_request() << ",sscale=" << h.spatial_scale() << ",tscale=" << h.temporal_scale() << ",lconf=" << ",lhaddr=" << h._last_hop_address << ",lht=" << h._last_hop_time << ",oaddr=" << h._origin_address << ",ot=" << h._origin_time << ",de=" << h._deadline << "}" << endl;
            return os;
        }

    private:
        union {
            unsigned char _cfg;
            struct {
                unsigned char _message_type : 3;
                unsigned char _time_request : 1;
                unsigned char _spatial_scale : 2;
                unsigned char _temporal_scale : 2;
            }__attribute__((packed));
        }__attribute__((packed));
        //unsigned _location_confidence : 8;
        Local_Address _last_hop_address;
        Time _last_hop_time;
        Local_Address _origin_address;
        Time _origin_time;
        Time _deadline;
    } __attribute__((packed));

    // Generic Frame. Should be parsed and cast to a Message
    class Frame : public Header {
    public:
        friend Debug & operator<<(Debug & db, const Frame & f) {
            db << *reinterpret_cast<const Header*>(&f);
            return db;
        }
        friend OStream & operator<<(OStream & os, const Frame & f) {
            os << *reinterpret_cast<const Header*>(&f);
            return os;
        }
        Header * header() { return this; }
        template<typename T>
        T* as() { return reinterpret_cast<T*>(this); }
    private:
        unsigned char _data[MTU - sizeof(Header)];
    }__attribute__((packed));

    class Interest_Message : public Header {
    public:
        static const MESSAGE_TYPE TYPE = MESSAGE_TYPE::INTEREST;

        Interest_Message() : Header(TYPE) { }
        Interest_Message(const Local_Address & origin, const Time & deadline, const Remote_Address & destination, 
                const Time & t0, const Time & t_end, const Time & period, const Unit unit, 
                const RESPONSE_MODE & response_mode, const Error & max_error) : 
            Header(TYPE, origin, deadline), _destination(destination), _t0(t0), _t_end(t_end), 
            _period(period), _unit(unit),  _response_mode(response_mode), _error(max_error) { }

        Time t0() { return _t0; }
        Time t_end() { return _t_end; }
        Time period() { return _period; }
        Unit unit() { return _unit; }
        Error error() { return _error; }
        const Remote_Address & destination() { return _destination; }

        friend Debug & operator<<(Debug & db, const Interest_Message & i) {
            db << *reinterpret_cast<const Header*>(&i) << ", dst=" << i._destination << ", t0=" << i._t0 << ", tend=" << i._t_end << ", p=" << i._period << ", u=" << i._unit << ", rm=" << i._response_mode << ", err=" << i._error;
            return db;
        }
        friend OStream & operator<<(OStream & os, const Interest_Message & i) {
            os << *reinterpret_cast<const Header*>(&i) << ", dst=" << i._destination << ", t0=" << i._t0 << ", tend=" << i._t_end << ", p=" << i._period << ", u=" << i._unit << ", rm=" << i._response_mode << ", err=" << i._error;
            return os;
        }
    private:
        Remote_Address _destination;
        Time _t0;
        Time _t_end;
        Time _period;
        Unit _unit;
        unsigned _response_mode : 1;
        unsigned _error : 7;
    }__attribute__((packed));

    class Buffer;
    class Scheduled_Message {
        Message_ID _id;
        Frame * _frame;
        Buffer * _buffer;
        Alarm * _alarm;
    };

    //typedef _UTIL::Buffer<TSTP, Frame, Scheduled_Message> Buffer;
    typedef _UTIL::Buffer<TSTP, Frame> Buffer;

    static const unsigned int MAX_DATA_SIZE = MTU - sizeof(Header) - sizeof(Sec_MAC);

    class Data_Message : public Header {
        friend class TSTP_Security;
    public:
        static const MESSAGE_TYPE TYPE = MESSAGE_TYPE::DATA;

        friend Debug & operator<<(Debug & db, const Data_Message & d) {
            db << *reinterpret_cast<const Header*>(&d) << ", mac=" << d._mac << ", u=" << d._unit << ", _data=" << reinterpret_cast<const void *>(d._data);
            return db;
        }
        friend OStream & operator<<(OStream & os, const Data_Message & d) {
            os << *reinterpret_cast<const Header*>(&d) << ", mac=" << d._mac << ", u=" << d._unit << ", _data=" << reinterpret_cast<const void *>(d._data);
            return os;
        }

        Data_Message() : Header(TYPE) { }
        Data_Message(Unit unit) : Header(TYPE), _unit(unit) { }

        Unit unit() { return _unit; }

        template<typename T = unsigned int> // TODO
        T* data() { return reinterpret_cast<T*>(_data); }

    protected:
        Sec_MAC _mac;
        Unit _unit;
        unsigned char _data[MAX_DATA_SIZE];
    }__attribute__((packed));

    class Report_Message : public Header {
    public:
        static const MESSAGE_TYPE TYPE = MESSAGE_TYPE::REPORT;
        Report_Message() : Header(TYPE) { }
    private:
        Sec_MAC _mac;
        Unit _unit;
        unsigned char _data[MAX_DATA_SIZE];
    }__attribute__((packed));
};

__END_SYS

#endif