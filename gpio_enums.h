#ifndef GPIO_ENUMS_H
#define GPIO_ENUMS_H

#define GPIO_PINS	54

enum TGPIOVirtualPin
{
	GPIOPinAudioLeft	= GPIO_PINS,
	GPIOPinAudioRight,
	GPIOPinUnknown
};

enum TGPIOClock
{
	GPIOClock0   = 0,			// on GPIO4 Alt0 or GPIO20 Alt5
	GPIOClock2   = 2,			// on GPIO6 Alt0
	GPIOClockPCM = 5,
	GPIOClockPWM = 6
};

enum TGPIOClockSource
{
	GPIOClockSourceOscillator = 1,		// 19.2 MHz
	GPIOClockSourcePLLC       = 5,		// 1000 MHz (changes with overclock settings)
	GPIOClockSourcePLLD       = 6,		// 500 MHz
	GPIOClockSourceHDMI       = 7		// 216 MHz
};

enum TGPIOMode
{
	GPIOModeInput,
	GPIOModeOutput,
	GPIOModeInputPullUp,
	GPIOModeInputPullDown,
	GPIOModeAlternateFunction0,
	GPIOModeAlternateFunction1,
	GPIOModeAlternateFunction2,
	GPIOModeAlternateFunction3,
	GPIOModeAlternateFunction4,
	GPIOModeAlternateFunction5,
	GPIOModeUnknown
};

enum TGPIOInterrupt
{
	GPIOInterruptOnRisingEdge,
	GPIOInterruptOnFallingEdge,
	GPIOInterruptOnHighLevel,
	GPIOInterruptOnLowLevel,
	GPIOInterruptOnAsyncRisingEdge,
	GPIOInterruptOnAsyncFallingEdge,
	GPIOInterruptUnknown
};

enum TMachineModel
{
	MachineModelA,
	MachineModelBRelease1MB256,
	MachineModelBRelease2MB256,
	MachineModelBRelease2MB512,
	MachineModelAPlus,
	MachineModelBPlus,
	MachineModelZero,
	MachineModelZeroW,
	MachineModel2B,
	MachineModel3B,
	MachineModel3BPlus,
	MachineModelCM,
	MachineModelCM3,
	MachineModelUnknown
};

#endif
