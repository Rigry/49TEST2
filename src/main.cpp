#define STM32F030x6
#define F_OSC   8000000UL
#define F_CPU   48000000UL
#define ADR(reg) GET_ADR(In_regs, reg)

#include "init_clock.h"
#include "periph_rcc.h"
#include "modbus_slave.h"
#include "timers.h"

extern "C" void init_clock() { init_clock<F_OSC, F_CPU>(); }

using DI1  = mcu::PA9 ;  using DO1  = mcu::PB0 ;
using DI2  = mcu::PA8 ;  using DO2  = mcu::PA7 ;
using DI3  = mcu::PB15;  using DO3  = mcu::PA6 ;
using DI4  = mcu::PB14;  using DO4  = mcu::PA5 ;
using DI5  = mcu::PB13;  using DO5  = mcu::PB5 ;
using DI6  = mcu::PB12;  using DO6  = mcu::PB4 ;
using DI7  = mcu::PB11;  using DO7  = mcu::PB3 ;
using DI8  = mcu::PB10;  using DO8  = mcu::PA15;
using DI9  = mcu::PB2 ;
using DI10 = mcu::PB1 ;

using TX_slave  = mcu::PA2;
using RX_slave  = mcu::PA3;
using RTS_slave = mcu::PA1;


int main()
{
   auto[do1,do2,do3,do4,do5,do6,do7,do8]
     = make_pins<mcu::PinMode::Output, DO1,DO2,DO3,DO4,DO5,DO6,DO7,DO8>();
   auto[di1,di2,di3,di4,di5,di6,di7,di8,di9,di10] 
      = make_pins<mcu::PinMode::Input, DI1,DI2,DI3,DI4,DI5,DI6,DI7,DI8,DI9,DI10>();
   
   struct Flash_data {
      uint16_t factory_number = 0;
      UART::Settings uart_set = {
         .parity_enable  = false,
         .parity         = USART::Parity::even,
         .data_bits      = USART::DataBits::_8,
         .stop_bits      = USART::StopBits::_1,
         .baudrate       = USART::Baudrate::BR9600,
         .res            = 0
      };
      uint8_t  modbus_address = 1;
   } flash;
   
   struct Sense {
      bool DI1  :1;  // Bit 0
      bool DI2  :1;  // Bit 1
      bool DI3  :1;  // Bit 2
      bool DI4  :1;  // Bit 3 
      bool DI5  :1;  // Bit 4
      bool DI6  :1;  // Bit 5
      bool DI7  :1;  // Bit 6
      bool DI8  :1;  // Bit 7 
      bool DI9  :1;  // Bit 8
      bool DI10 :1;  // Bit 9
      uint16_t  :6;  // Bits 15:10 res: Reserved, must be kept cleared
   };

   struct Control {
      bool DO1  :1;  // Bit 0
      bool DO2  :1;  // Bit 1
      bool DO3  :1;  // Bit 2
      bool DO4  :1;  // Bit 3 
      bool DO5  :1;  // Bit 4
      bool DO6  :1;  // Bit 5
      bool DO7  :1;  // Bit 6
      bool DO8  :1;  // Bit 7 
      uint16_t  :8;  // Bits 15:8 res: Reserved, must be kept cleared
   };
   
   
   struct In_regs {
   
      UART::Settings uart_set;   // 0
      uint16_t modbus_address;   // 1
      uint16_t password;         // 2
      uint16_t factory_number;   // 3
      Control   output;          // 4 
   
   }__attribute__((packed));


   struct Out_regs {
   
      uint16_t device_code;      // 0
      uint16_t factory_number;   // 1
      UART::Settings uart_set;   // 2
      uint16_t modbus_address;   // 3
      Sense    input;            // 4
      uint16_t qty_input;        // 5
      uint16_t qty_output;       // 6

   }__attribute__((packed));

   auto& slave = Modbus_slave<In_regs, Out_regs>::
      make<mcu::Periph::USART1, TX_slave, RX_slave, RTS_slave>
         (flash.modbus_address, flash.uart_set);


   slave.outRegs.device_code        = 94; // test_version
   slave.outRegs.factory_number     = flash.factory_number;
   slave.outRegs.uart_set           = flash.uart_set;
   slave.outRegs.modbus_address     = flash.modbus_address;
   slave.outRegs.qty_input          = 10;
   slave.outRegs.qty_output         = 8;


   // auto& uart = REF(USART1);
   // like_CMSIS(uart).CR1 |= USART_CR1_RXNEIE_Msk;

   
   // auto& rts = Pin::make<RTS_slave, mcu::PinMode::Output>();
   // rts = true;
   // Timer timer {1000};
   // Timer timer1 {10};
   while(1){

      // if (timer1.event()){
      //    like_CMSIS(uart).TDR = 1;
      // }
      // do4 = do1 ^= timer.event();
      // do2 = do3 ^= timer1.event();
      // slave reaction
      slave ([&](uint16_t registrAddress) {
            static bool unblock = false;
         switch (registrAddress) {
            case ADR(uart_set):
               flash.uart_set
                  = slave.outRegs.uart_set
                  = slave.inRegs.uart_set;
            break;
            case ADR(modbus_address):
               flash.modbus_address 
                  = slave.outRegs.modbus_address
                  = slave.inRegs.modbus_address;
            break;
            case ADR(password):
               unblock = slave.inRegs.password == 208;
            break;
            case ADR(factory_number):
               if (unblock) {
                  unblock = false;
                  flash.factory_number 
                     = slave.outRegs.factory_number
                     = slave.inRegs.factory_number;
               }
               unblock = true;
            break;
            case ADR(output):
               do1 = slave.inRegs.output.DO1;
               do2 = slave.inRegs.output.DO2;
               do3 = slave.inRegs.output.DO3;
               do4 = slave.inRegs.output.DO4;
               do5 = slave.inRegs.output.DO5;
               do6 = slave.inRegs.output.DO6;
               do7 = slave.inRegs.output.DO7;
               do8 = slave.inRegs.output.DO8;
            break;
         }

      });

      slave.outRegs.input.DI1 = di1;
      slave.outRegs.input.DI2 = di2;
      slave.outRegs.input.DI3 = di3;
      slave.outRegs.input.DI4 = di4;
      slave.outRegs.input.DI5 = di5;
      slave.outRegs.input.DI6 = di6;
      slave.outRegs.input.DI7 = di7;
      slave.outRegs.input.DI8 = di8;
      slave.outRegs.input.DI9 = di9;
      slave.outRegs.input.DI10 = di10;

      __WFI();

   } //while
}