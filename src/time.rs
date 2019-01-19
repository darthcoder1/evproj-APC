#![deny(unsafe_code)]

extern crate cortex_m;
use cortex_m::peripheral::DWT;
extern crate stm32f103xx_hal as hal;

use hal::prelude::*;
use hal::stm32f103xx;

// ---------------
// Time Handling code
pub use hal::rcc::Clocks;

#[derive(Clone, Copy)]
pub struct TimeStamp(pub u32);

pub fn device_get_ticks() -> TimeStamp {
	
	TimeStamp(DWT::get_cycle_count())
}

pub fn device_get_clock_frequency() -> u32 {

	let _stm32f103 = stm32f103xx::Peripherals::take().unwrap();
	let mut _rcc = _stm32f103.RCC.constrain();
	let mut _flash = _stm32f103.FLASH.constrain();
	let _clocks = _rcc.cfgr.freeze(& mut _flash.acr);

	_clocks.sysclk().0
}

pub fn time_us_to_ticks(_clocks : &Clocks, us : u32) -> u32 {
	us *(_clocks.sysclk().0 / 1000000)
}

pub fn time_ms_to_ticks(_clocks : &Clocks, ms : u32) -> u32 {
	time_us_to_ticks(&_clocks, ms * 1000)
}