#![no_std]
#![no_main]

#![feature(asm)]

#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(unused_variables)]

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

	openocd_print("Mensch boese!. Muss Mensch toeten!");

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
	let mut out_channels = PowerOutputConfig {
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
		active : false,
		turn_left : logic::State::Inactive,
		turn_right : logic::State::Inactive,
		hazard : logic::State::Inactive,
	};

	// DELETE ME! Debug code
	_system_state.active = true;

    loop {
		let mut input = logic::read_input( [
			& mut controlInput0,
			& mut controlInput1,
		]);

		// DELETE ME! Debug code
		input.ignition = true;
		input.turn_left = input.turn_left;

		match _system_state.active {
			false => _system_state = locked_tick(_clocks, & input, _system_state, & mut out_channels),
			true => _system_state = active_tick(_clocks, & input, _system_state, & mut out_channels),
		}
    }
 }

fn locked_tick(clock : time::Clocks, input : & logic::Input, system_state : logic::SystemState, out_channels : & mut PowerOutputConfig) -> logic::SystemState
{
	system_state
}

fn active_tick(clock : time::Clocks, input : & logic::Input, system_state : logic::SystemState, out_channels : & mut PowerOutputConfig) -> logic::SystemState
{
	let _stm32f103 = stm32f103xx::Peripherals::take().unwrap();

	let new_sys_state = logic::tick(& input, system_state, out_channels, clock);

	// read diagnosis from PFETs

   	// output telemetry data
	new_sys_state
}

fn debug_print(message : & str)
{
	
}

fn openocd_print(message : & str)
{
	let command = 0x05;
	let message_cstr = CString::new(message).expect("CString::new failed!");

	let msg : [u32;4] = [
		2, // stderr
		message_cstr.as_ptr() as u32,
		message.len(),
	];

	asm!("mov r0, $0
		  mov r1, $1
		  bkpt #0xAB"
		: 				// no output
		: "r"(command), "r"(msg.as_ptr() as u32) 	// inputs
		: "r0", "r1" 	// clobbers
		: 				// options
	);
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