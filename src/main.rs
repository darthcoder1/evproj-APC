#![no_std]
#![no_main]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

extern crate cortex_m;
extern crate cortex_m_rt as rt;
extern crate stm32f103xx_hal as hal;
extern crate embedded_hal;

mod time;
mod logic;

//use rt::{entry, exception, ExceptionFrame};
use rt::{entry, exception, ExceptionFrame};
use hal::prelude::*;
use hal::stm32f103xx;
use logic::{ 
	PowerOutputConfig,
	DriverControlConfig
};
use embedded_hal::digital::OutputPin;
use cortex_m::{asm, interrupt};
use core::panic::PanicInfo;

#[entry]
fn main() -> ! {

	let mut _cortex_m = cortex_m::Peripherals::take().unwrap();
	let _stm32f103 = stm32f103xx::Peripherals::take().unwrap();

	let mut _flash = _stm32f103.FLASH.constrain();
	let mut _rcc = _stm32f103.RCC.constrain();

	let _clocks = _rcc.cfgr.freeze(& mut _flash.acr);
	_cortex_m.DWT.enable_cycle_counter();

	let mut gpioa = _stm32f103.GPIOA.split(& mut _rcc.apb2);
	let mut gpiob = _stm32f103.GPIOB.split(& mut _rcc.apb2);
	let mut gpioc = _stm32f103.GPIOC.split(& mut _rcc.apb2);

	let mut led = gpioc.pc13.into_push_pull_output(& mut gpioc.crh); led.set_high();
	
	// input logic mapping
	let controlInput0_Data = gpiob.pb4.into_floating_input(& mut gpiob.crl);
	let mut controlInput0_Sel0 = gpioa.pa11.into_push_pull_output(& mut gpioa.crh);
	let mut controlInput0_Sel1 = gpioa.pa12.into_push_pull_output(& mut gpioa.crh);
	let mut controlInput0_Sel2 = gpioa.pa13.into_push_pull_output(& mut gpioa.crh);
	
	let controlInput1_Data = gpiob.pb3.into_floating_input(& mut gpiob.crl);
	let mut controlInput1_Sel0 = gpioa.pa10.into_push_pull_output(& mut gpioa.crh);
	let mut controlInput1_Sel1 = gpioa.pa9.into_push_pull_output(& mut gpioa.crh);
	let mut controlInput1_Sel2 = gpioa.pa8.into_push_pull_output(& mut gpioa.crh);

	// Hardware control mapping
	// These are the 2 mulitplexor chips, the first one is responsible
	// for reading the first 8 channels, the 2nd for the other 8 Channels.
	// To read thenm select the line 

	// input channels 0 - 7
	let mut controlInput0 =	DriverControlConfig::new(
			& controlInput0_Data,
			[ & mut controlInput0_Sel0 as & mut OutputPin,
			  & mut controlInput0_Sel1 as & mut OutputPin,
			  & mut controlInput0_Sel2 as & mut OutputPin ]
		);
	// driver controls input channels 8 - 15
	let mut controlInput1 = DriverControlConfig::new(
			& controlInput1_Data,
			[ & mut controlInput1_Sel0 as & mut OutputPin,
			  & mut controlInput1_Sel1 as & mut OutputPin,
			  & mut controlInput1_Sel2 as & mut OutputPin ]
		);

	// This is the mapping between the actual pins and the power channels they switch
	
	// acquire all the pins 
	let mut channel01 = gpioa.pa6.into_push_pull_output(& mut gpioa.crl);	channel01.set_low();
	let mut channel02 = gpioa.pa7.into_push_pull_output(& mut gpioa.crl);	channel02.set_low();
	let mut channel03 = gpiob.pb0.into_push_pull_output(& mut gpiob.crl);	channel03.set_low();
	let mut channel04 = gpiob.pb1.into_push_pull_output(& mut gpiob.crl);	channel04.set_low();
	let mut channel05 = gpiob.pb10.into_push_pull_output(& mut gpiob.crh);	channel05.set_low();
	let mut channel06 = gpiob.pb11.into_push_pull_output(& mut gpiob.crh);	channel06.set_low();

	let mut channel07 = gpioa.pa5.into_push_pull_output(& mut gpioa.crl);	channel07.set_low();
	let mut channel08 = gpioa.pa4.into_push_pull_output(& mut gpioa.crl);	channel08.set_low();
	let mut channel09 = gpioa.pa3.into_push_pull_output(& mut gpioa.crl);	channel09.set_low();
	let mut channel10 = gpioa.pa2.into_push_pull_output(& mut gpioa.crl);	channel10.set_low();
	let mut channel11 = gpioa.pa1.into_push_pull_output(& mut gpioa.crl); 	channel11.set_low();
	let mut channel12 = gpioa.pa0.into_push_pull_output(& mut gpioa.crl);  	channel11.set_low();

	// create the mapping
	let mut outChannels = PowerOutputConfig {
		channels: [
			& mut channel01,
			& mut channel02,
			& mut channel03,
			& mut channel04,
			& mut channel05,
			& mut channel06,
			& mut channel07,
			& mut channel08,
			& mut channel09,
			& mut channel10,
			& mut channel11,
			& mut channel12,
		]
	};



	// setup the turn signal state
	let mut _system_state = logic::SystemState {
		turn_left : logic::State::Inactive,
		turn_right : logic::State::Inactive,
		hazard : logic::State::Inactive,
	};


    loop {
		let mut input = logic::read_input( [
			& mut controlInput0,
			& mut controlInput1,
		]);

		input.ignition = true;
		input.turn_left = input.turn_left;

		_system_state = logic::tick(& input, _system_state, & mut outChannels, _clocks);

		// read diagnosis from PFETs

    	// output telemetry data
    }
 }

#[panic_handler]
unsafe fn panic(info: &PanicInfo) -> ! {
    interrupt::disable();
    asm::bkpt();

	loop {}
}


// define the hard fault handler
#[exception]
fn HardFault(_ef: &ExceptionFrame) -> ! {
     asm::bkpt();

	 loop {}
 }