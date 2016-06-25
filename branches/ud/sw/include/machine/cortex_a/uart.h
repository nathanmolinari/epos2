// EPOS Cortex_A UART Mediator Declarations

#include __MODEL_H

#ifndef __cortex_a_uart_h__
#define __cortex_a_uart_h__

#include <cpu.h>
#include <uart.h>

__BEGIN_SYS

class Cortex_A_UART: public UART_Common, private Cortex_A_Model_UART
{
private:
    typedef Cortex_A_Model_UART Engine;

    static const unsigned int BAUD_RATE = Traits<Cortex_A_UART>::DEF_BAUD_RATE;
    static const unsigned int DATA_BITS = Traits<Cortex_A_UART>::DEF_DATA_BITS;
    static const unsigned int PARITY = Traits<Cortex_A_UART>::DEF_PARITY;
    static const unsigned int STOP_BITS = Traits<Cortex_A_UART>::DEF_STOP_BITS;

public:
    // The default unit is 1 because Serial_Display instantiate it without
    // paremeters and we want it to use UART 1. By default, the UART 0 is
    // disabled in Vivado.
    Cortex_A_UART(unsigned int baud_rate = BAUD_RATE, unsigned int data_bits = DATA_BITS, unsigned int parity = PARITY, unsigned int stop_bits = STOP_BITS, unsigned int unit = 1)
    : Engine(unit, baud_rate, data_bits, parity, stop_bits) {}

    void config(unsigned int baud_rate, unsigned int data_bits, unsigned int parity, unsigned int stop_bits) {
        Engine::config(baud_rate, data_bits, parity, stop_bits);
    }
    void config(unsigned int * baud_rate, unsigned int * data_bits, unsigned int * parity, unsigned int * stop_bits) {
        Engine::config(*baud_rate, *data_bits, *parity, *stop_bits);
    }

    char get() { while(!rxd_ok()); return rxd(); }
    void put(char c) { while(!txd_ok()); txd(c); }

    bool ready_to_get() { return rxd_ok(); }
    bool ready_to_put() { return txd_ok(); }

    //void int_enable(bool receive = true, bool send = true, bool line = true, bool modem = true) {
        //Engine::int_enable(receive, send, line, modem);
    //}
    //void int_disable(bool receive = true, bool send = true, bool line = true, bool modem = true) {
        //Engine::int_disable(receive, send, line, modem);
    //}

    void reset() { Engine::config(BAUD_RATE, DATA_BITS, PARITY, STOP_BITS); }
    void loopback(bool flag) { Engine::loopback(flag); }
};

__END_SYS

#endif
