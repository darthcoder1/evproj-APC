#![allow(non_snake_case)]

pub use time::TimeStamp;

use time;
use embedded_hal::digital::OutputPin;
use embedded_hal::digital::InputPin;

// This indicates s state and if active, since which tick
pub enum State
{
	Active(TimeStamp),
	Inactive,
}

// This is the system state that needs to be kept around for timing relevant
// state
pub struct SystemState
{
	// indicates whether the system is in locked or active
	pub active : bool,

    pub turn_left : State,
	pub turn_right : State,
	pub hazard : State,
}

// These are the inputs coming from the driver controlls
pub struct Input
{
	// true when the ignition is enabled
	pub ignition : bool,
	// true when front brake is used
	pub brake_front : bool,
	// true when back brake is used
	pub brake_rear : bool,
	// true when the left turn signal is activated
	pub turn_left : bool,
	// true when the right turn signal is activated
	pub turn_right : bool,
	// true when hazard light is turned on
	pub hazard_light : bool,
	// true when the lights are turned on
	pub light_on : bool,
	// true when the full beam is turned on
	pub full_beam : bool,
	// true when horn button pressed
	pub horn : bool,
	// true when killswitch is on KILL
	pub kill_switch : bool,
	// true when sidestand is out
	pub side_stand : bool,
}


// This is used to configure the Input Hardware
// 2 multiplexers are controlling 8 input channels each.
// This data structure is used to configure one multiplexer.
// It specifies the data and the selector pins and also defines
// the selector pin states to read from a specific channel
//
// Ex. selectors[0]: [0,1,0] 
// The above code means, to read from channel 0, set the selectors
// to low, high, low.
pub struct DriverControlConfig<'a>
{
	dataPin 		: &'a InputPin,
	selectPins 		: [&'a mut OutputPin;3],
	selectors 		: [[i32;3];8]
}

impl<'a> DriverControlConfig<'a>
{
	pub fn new<'b>(dataPin : &'a InputPin, selectPins : [&'a mut OutputPin;3]) -> DriverControlConfig<'a> {
		DriverControlConfig {
			dataPin: dataPin,
			selectPins : selectPins,
			selectors: [
				[1,1,0], 		// Y3
				[0,0,0], 		// Y0
				[1,0,0], 		// Y1
				[0,1,0], 		// Y2
				[1,0,1], 		// Y5
				[1,1,1], 		// Y7
				[0,1,1], 		// Y6
				[0,0,1], 		// Y4
			]
		}
	}

	pub fn ReadChannel(& mut self, channelIdx : usize) -> Option<bool> {
		if channelIdx >= 8 {
			None
		} else {
		
			let channelSelectors = self.selectors[channelIdx];
		
			for i in 0..3 {
				SetOutputPin(self.selectPins[i], channelSelectors[i]==1);
			}
			
			Some(self.dataPin.is_high())
		}
	}
}

pub struct PowerOutputConfig<'a>
{
	pub channels : [&'a mut OutputPin;12],
}

impl<'a> PowerOutputConfig<'a> 
{
	pub fn SwitchChannel(& mut self, channelIdx : usize, isHigh : bool) {
		SetOutputPin(self.channels[channelIdx], isHigh);
	}
}


fn SetOutputPin(pin : & mut OutputPin, high : bool) {
	if high {
		pin.set_high();
	} else {
		pin.set_low();
	}
}

//fn initialize_power_channel(_channel : PowerChannel, )

// This describes how the power ouptput needs to be swithed, which
// outputs to open and which to close
pub struct PowerOutput
{
	turn_left_front : bool,
	turn_left_rear : bool,
	turn_right_front : bool,
	turn_right_rear : bool,
	head_light_parking : bool,
	head_light_lowbeam : bool,
	head_light_fullbeam : bool,
	rear_light : bool,
	brake_light : bool,
	horn : bool,
    _unused0 : bool,
    _unused1 : bool,
}


// Reads the input from the control input pins
// it expectes an array of DriverControlConfigs, where
// each element describes the access to one multiplexer, 
// which allows to read input from 8 channels.
pub fn read_input( controlConfigs : [& mut DriverControlConfig;2] ) -> Input {

	Input {
		ignition : DriverControlConfig::ReadChannel(controlConfigs[0], 1).unwrap(),
		brake_front : DriverControlConfig::ReadChannel(controlConfigs[1], 0).unwrap(),
		brake_rear : DriverControlConfig::ReadChannel(controlConfigs[1], 1).unwrap(),
		turn_left : DriverControlConfig::ReadChannel(controlConfigs[0], 5).unwrap(),
		turn_right : DriverControlConfig::ReadChannel(controlConfigs[0], 6).unwrap(),
		hazard_light : DriverControlConfig::ReadChannel(controlConfigs[0], 7).unwrap(),
		light_on : DriverControlConfig::ReadChannel(controlConfigs[0], 3).unwrap(),
		full_beam : DriverControlConfig::ReadChannel(controlConfigs[0], 4).unwrap(),
		horn : DriverControlConfig::ReadChannel(controlConfigs[1], 2).unwrap(), 
		kill_switch :  DriverControlConfig::ReadChannel(controlConfigs[0], 0).unwrap(),
		side_stand : DriverControlConfig::ReadChannel(controlConfigs[0], 2).unwrap(),
	}
}


fn update_state(_prev_state : State, _input_flag : bool) -> State {
	
	if _input_flag  {
		match _prev_state {
			State::Active(_time) => _prev_state,
			State::Inactive => State::Active(time::device_get_ticks()),
		}
	}
	else {
		State::Inactive
	}
}

fn update_system_state(_system : SystemState, _input : &Input) -> SystemState {

	SystemState {
		active : _system.active,
        turn_left : update_state(_system.turn_left, _input.turn_left),
	    turn_right : update_state(_system.turn_right, _input.turn_right),
	    hazard : update_state(_system.hazard, _input.hazard_light)
    }
}

fn caclulate_turn_signal(_state : &State, _cur_time : TimeStamp, _on_time : u32, _off_time : u32) -> bool {

	match _state {
		State::Active(start_time) => {
			let _time_passed = _cur_time.0.wrapping_sub(start_time.0);
			let _passed_cycles_mod = _time_passed % (_on_time + _off_time);

			if _passed_cycles_mod < _on_time {
				true
			}
			else {
				false
			}
		},
		State::Inactive => false,
	}
}

// Switch the turn signals based on the driver input and also calculate the signaling based on the 
// defined turn signal interval.
//
// Order of evaluations:
//  * Hazard:		If hazard is activated, turn signal input is ignored, and all turn signal lights blink with
//					the defined frequency
//  * Turn_Left/
//	  Turn_Right:	If turn signal is activated, the turn signal lights on the according side will blink
//					with the defined frequency
fn switch_turn_signals(_system_state : &SystemState, _input : &Input, _clocks : &time::Clocks, _power_out : & mut PowerOutput)
{
	let current_time = time::device_get_ticks();
	let _one_sec_in_ticks = time::time_ms_to_ticks(&_clocks, 1000);

	let _hazard_on = match _system_state.hazard {
		State::Active(_start_time) => (true, caclulate_turn_signal(&_system_state.hazard, current_time, _one_sec_in_ticks, _one_sec_in_ticks)),
		State::Inactive => (false, false),
	};
	
	let _turn_left_on = match _system_state.turn_left {
		State::Active(_start_time) => (true, caclulate_turn_signal(&_system_state.turn_left, current_time, _one_sec_in_ticks, _one_sec_in_ticks)),
		State::Inactive => (false, false),
	};
	
	let _turn_right_on = match _system_state.turn_right {
		State::Active(_start_time) => (true, caclulate_turn_signal(&_system_state.turn_right, current_time, _one_sec_in_ticks, _one_sec_in_ticks)),
		State::Inactive => (false, false),
	};

	if _hazard_on.0 {
		_power_out.turn_right_front = _hazard_on.1;
		_power_out.turn_right_rear = _hazard_on.1;
		_power_out.turn_left_front = _hazard_on.1;
		_power_out.turn_left_rear = _hazard_on.1;
	}
	else {
		if _turn_left_on.0 {
			_power_out.turn_left_front = _turn_left_on.1;
			_power_out.turn_left_rear = _turn_left_on.1;
			_power_out.turn_right_front = false;
			_power_out.turn_right_rear = false;
		}
		else if _turn_right_on.0 {
			_power_out.turn_right_front = _turn_right_on.1;
			_power_out.turn_right_rear = _turn_right_on.1;
			_power_out.turn_left_front = false;
			_power_out.turn_left_rear = false;
			
		}
		else {
			_power_out.turn_right_front = false;
			_power_out.turn_right_rear = false;	
			_power_out.turn_left_front = false;
			_power_out.turn_left_rear = false;
		}
	}
}

// Switches the lights according to driver input
//
// There are two modes:
//  * Ignition On:		When light is turned on, low beam and rear light will
//						be activated. This also allows the full beam to be turned on
//						when the driver input activated it.
//						When light is off, all head and read lights are turned off and
//						the full beam input will be ignored.
//  * Ignition Off:		When light is on, parking light is enabled, all other ligths stay off.
fn switch_light_signals(_input : &Input, _power_out : & mut PowerOutput) {

	if _input.ignition {
	
		match _input.light_on {
			true => {
				_power_out.head_light_lowbeam = true;
				_power_out.rear_light = true;
				_power_out.head_light_fullbeam = _input.full_beam;
			}
			false => {
				_power_out.head_light_lowbeam = false;
				_power_out.rear_light = false;
			}
		}
	} else {
		match _input.light_on {
			true => {
				_power_out.head_light_lowbeam = false;
				_power_out.head_light_parking = true;
				_power_out.rear_light = true;
			},
			false => {
				_power_out.head_light_lowbeam = false;
				_power_out.head_light_parking = false;
				_power_out.rear_light = false;
			}
		}
		_power_out.head_light_fullbeam = false;
	}	
}


// Switches the power output based on the input and current system state and
// returns the PowerOutput prefilled. The return value contains the state of the
// Power output as it should be applied by the hardware. 
fn switch_power_output(_system : &SystemState, _input : &Input, _clock : &time::Clocks) -> PowerOutput {
	
	let mut power_output = PowerOutput {
		turn_left_front : false,
		turn_left_rear : false,
		turn_right_front : false,
		turn_right_rear : false,
		head_light_parking : false,
		head_light_lowbeam : false,
		head_light_fullbeam : false,
		rear_light : false,
		brake_light : _input.brake_front || _input.brake_rear,
		horn : _input.horn,
        _unused0 : false,
        _unused1 : false,
	};

	switch_turn_signals(&_system, &_input, &_clock, & mut power_output);	
	switch_light_signals(&_input, & mut power_output);

	power_output
}

fn apply_power_output(powerOut : PowerOutput, powerChannels : & mut PowerOutputConfig) {
	
	powerChannels.SwitchChannel(0,  powerOut.turn_left_front);
	powerChannels.SwitchChannel(1,  powerOut.head_light_fullbeam);
	powerChannels.SwitchChannel(2,  powerOut.head_light_parking);
	powerChannels.SwitchChannel(3,  powerOut.turn_left_front);
	powerChannels.SwitchChannel(4,  powerOut.turn_left_rear);
	powerChannels.SwitchChannel(5,  powerOut.turn_right_front);
	powerChannels.SwitchChannel(6,  powerOut.turn_right_rear);
	powerChannels.SwitchChannel(7,  powerOut.rear_light);
	powerChannels.SwitchChannel(8,  powerOut.brake_light);
	powerChannels.SwitchChannel(9,  powerOut.horn);

	powerChannels.SwitchChannel(10, false);
	powerChannels.SwitchChannel(11, false);
}

pub fn tick(input : & Input, _system_state : SystemState, channelConfig : & mut PowerOutputConfig, _clocks : time::Clocks) -> SystemState
{
    let _new_system_state = update_system_state(_system_state, input);
    let _power_out = switch_power_output(&_new_system_state, input, &_clocks);
    
    apply_power_output(_power_out, channelConfig);
	_new_system_state
}