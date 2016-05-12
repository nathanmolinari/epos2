// EPOS Smart Data Declarations

#ifndef __smart_data_h
#define __smart_data_h

#include <tstp.h>
#include <periodic_thread.h>

__BEGIN_SYS

template <typename T>
struct Smart_Data_Type_Wrapper
{
   typedef T Type;
};

// Smart Data encapsulates Transducers (i.e. sensors and actuators), local or remote, and bridges them with TSTP
// Transducers must implement either sense() or actuate() and must define UNIT, NUM, and ERROR.
template<typename Transducer>
class Smart_Data: private TSTP::Observer, private Transducer::Observer
{
    friend class Smart_Data_Type_Wrapper<Transducer>::Type; // friend S is OK in C++11, but this GCC does not implements it yet. Remove after GCC upgrade.

private:
    typedef TSTP::Buffer Buffer;
    typedef typename TSTP::Responsive Responsive;
    typedef typename TSTP::Interested Interested;

public:
    static const unsigned int UNIT = Transducer::UNIT;
    static const unsigned int NUM = Transducer::NUM;
    static const unsigned int ERROR = Transducer::ERROR;
    typedef typename TSTP::Unit::Get<NUM>::Type Value;

    enum Mode {
        PRIVATE = 0,
        ADVERTISED = 1,
        COMMANDED = 3
    };

    static const unsigned int NO_DEVICE = -1;

    //typedef RTC::Microsecond Microsecond;

    typedef TSTP::Unit Unit;
    typedef TSTP::Error Error;
    typedef TSTP::Coordinates Coordinates;
    typedef TSTP::Region Region;
    typedef TSTP::Microsecond Microsecond;
    typedef Microsecond Time;

public:
    // Local data source, possibly advertised to or commanded by the network
    Smart_Data(unsigned int dev, const Microsecond & expiry, const Mode & mode = PRIVATE)
    : _unit(UNIT), _value(0), _error(ERROR), _coordinates(TSTP::here()), _time(0), _expiry(expiry), _device(dev), _mode(mode), _thread(0), _interested(0), _responsive((mode & ADVERTISED) | (mode & COMMANDED) ? new Responsive(this, UNIT, ERROR, expiry) : 0) {
        db<Smart_Data>(TRC) << "Smart_Data(dev=" << dev << ",exp=" << expiry << ",mode=" << mode << ")" << endl;
        if(Transducer::POLLING) {
            Transducer::sense(_device, this);
            _time = TSTP::now();
        }
        if(Transducer::INTERRUPT)
            Transducer::attach(this);
        if(_responsive)
            TSTP::attach(this, _responsive);
        db<Smart_Data>(INF) << "Smart_Data(dev=" << dev << ",exp=" << expiry << ",mode=" << mode << ") => " << *this << endl;
    }
    // Remote, event-driven (period = 0) or time-triggered data source
    Smart_Data(const Region & region, const Microsecond & expiry, const Microsecond & period = 0)
    : _unit(UNIT), _value(0), _error(ERROR), _coordinates(0), _time(0), _expiry(expiry), _device(NO_DEVICE), _mode(PRIVATE), _thread(0), _interested(new Interested(this, region, UNIT, TSTP::SINGLE, 0, expiry, period)), _responsive(0) {
        TSTP::attach(this, _interested);
    }

    ~Smart_Data() {
        if(_thread)
            delete _thread;
        if(_interested) {
            TSTP::detach(this, _interested);
            delete _interested;
        }
        if(_responsive) {
            TSTP::detach(this, _responsive);
            delete _responsive;
        }
    }

    operator Value() {
        if(expired())
            if(_device != NO_DEVICE) { // Local data source
                Transducer::sense(_device, this); // read sensor
                _time = TSTP::now();
            } else { // Other data sources must have called update() timely
                db<Smart_Data>(WRN) << "Smart_Data::get(this=" << this << ",exp=" << _expiry << ",val=" << _value << ") => expired!" << endl;
            }
        return _value;
    }

    Time time() const { return _time; }
    bool expired() { return TSTP::now() > (_time + _expiry); }

    const Coordinates & origin() const { return TSTP::absolute(_coordinates); }

    friend Debug & operator<<(Debug & db, const Smart_Data & d) {
        db << "{";
        if(d._device != NO_DEVICE) {
            switch(d._mode) {
            case PRIVATE:    db << "PRI."; break;
            case ADVERTISED: db << "ADV."; break;
            case COMMANDED:  db << "CMD."; break;
            }
            db << "[" << d._device << "]:";
        }
        if(d._thread) db << "ReTT";
        if(d._responsive) db << "ReED";
        if(d._interested) db << "In" << ((d._interested->period()) ? "TT" : "ED");
        db << ":u=" << d._unit << ",v=" << d._value << ",e=" << int(d._error) << ",c=" << d._coordinates << ",t=" << d._time << ",x=" << d._expiry << "}";
        return db;
    }

private:
    void time(const Time & t) { _time = t; }

    void update(TSTP::Observed * obs, int subject, TSTP::Buffer * buf) {
        TSTP::Packet * packet = buf->frame()->data<TSTP::Packet>();
        db<Smart_Data>(TRC) << "Smart_Data::update(obs=" << obs << ",cond=" << reinterpret_cast<void *>(subject) << ",data=" << packet << ")" << endl;
        switch(packet->type()) {
        case TSTP::INTEREST: {
            TSTP::Interest * interest = reinterpret_cast<TSTP::Interest *>(packet);
            db<Smart_Data>(INF) << "Smart_Data::update[I]:msg=" << interest << " => " << *interest << endl;
            if(interest->period()) {
                if(!_thread)
                    _thread = new Periodic_Thread(interest->period(), &updater, _device, interest->expiry(), this, interest->region().t1);
                else {
                    /*
                    if(!interest->period() != _thread->period())
                        _thread->period(interest->period()) ;
                     */
                }
            } else {
                Transducer::sense(_device, this);
                _time = TSTP::now();
                _responsive->value(_value);
                _responsive->time(_time);
                _responsive->respond(interest->expiry());
            }
        } break;
        case TSTP::RESPONSE: {
            TSTP::Response * response = reinterpret_cast<TSTP::Response *>(packet);
            db<Smart_Data>(INF) << "Smart_Data:update[R]:msg=" << response << " => " << *response << endl;
            _value = response->value<Value>();
            _error = response->error();
            _coordinates = response->origin();
            _time = response->time();
            db<Smart_Data>(INF) << "Smart_Data:update[R]:this=" << this << " => " << *this << endl;
        }
        case TSTP::COMMAND: {
            if(_mode & COMMANDED) {
                TSTP::Command * command = reinterpret_cast<TSTP::Command *>(packet);
                Transducer::actuate(_device, this, command->command<void>());
            }
        } break;
        case TSTP::CONTROL: {
//            if(subtype == DELETE) { // Interest being revoked
//                delete _thread;
//                _thread = 0;
//            }
        } break;
        }
    }

    void update(typename Transducer::Observed * obs) {
        Transducer::sense(_device, this);
        _time = TSTP::now();
        db<Smart_Data>(TRC) << "Smart_Data::update(this=" << this << ",exp=" << _expiry << ") => " << _value << endl;
        db<Smart_Data>(TRC) << "Smart_Data::update:responsive=" << _responsive << " => " << *reinterpret_cast<TSTP::Response *>(_responsive) << endl;
        if(_responsive) {
            _responsive->value(_value);
            _responsive->time(_time);
            _responsive->respond(_expiry);
        }
    }

    static int updater(unsigned int dev, Time expiry, Smart_Data * data, Time end) {
        while(1) {
            Transducer::sense(dev, data);
            data->time(TSTP::now());
            data->_responsive->value(data->_value);
            data->_responsive->time(data->time());
            if(data->time() < end) {
                data->_responsive->respond(expiry);
                Periodic_Thread::wait_next();
            } else {
                return 0;
            }
        }
        return 0;
    }

private:
    Unit _unit;
    Value _value;
    Error _error;
    Coordinates _coordinates;
    Time _time;
    Time _expiry;

    unsigned int _device;
    Mode _mode;
    Periodic_Thread * _thread;
    Interested * _interested;
    Responsive * _responsive;
};

__END_SYS

#endif

#include <transducer.h>
