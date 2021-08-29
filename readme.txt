Built using Arduino Compatible Cross Platform C++ Library Project : For more information see http://www.visualmicro.com
blog: http://www.visualmicro.com/post/2017/01/16/Arduino-Cross-Platform-Library-Development.aspx

This project is designed to build an oiler system for a metal lathe. A base solution using this library has one or more pump motors used to deliver oil. Each motor must have a feedback signal to indicate the oil drips being delivered. The library counts drips delivered per motor and idles the motor when a configurable target (per motor) is met. The motors are restarted when a restart event is triggered. In the base solution the restart is a time event (in elapsed seconds) after the motor starts idling.

The library optionally also supports two additional input signals designed to be fed by the lathe being oiled. These signals are a pulse every time the lathe completes a revolution and a signal that is held HIGH or LOW (as configured) whilst the lathe is powered on. These two signals can be used as motor restart triggers i.e. restart oiling after so many revolutions or so many seconds of being active (ie powered on).

The library has support for two types of motors one driven by a simple relay switch and the other a stepper motor. Multiple motors of each type can be configured in any combination. The limits are the number of pins available on the Uno and a software limit of 6 motors. Since each motor needs a feedback signal as described above each relay based motor will use 2 Uno pins and each stepper motor 5 pins (the code is written for a 4 pin stepper driver).

Whilst the original purpose of this library is oiling the code has no real knowledge of the actual purpose. It switches motors on based on a trigger event and off after it gets enough signals that the delivery is complete. As a result this can be used for other purposes.

Motors track when to start and stop independently so a slow motor will not impact other motors.

When a motor is deciding when to restart it compares the state of the oiler at the time it last started - not the time it last stopped. This is done to reduce the impact of  slow motor, e.g. if oiling every 30 seconds but a slow oiler takes 12 seconds to oil, it will restart the slow motor 30 seconds after it started. If it started counting 30 seconds from when the oiling finished it would result in the next 30 + 12 seconds between oiling not 30 seconds. One side effect of this is that is a slow motor takes longer than the 30 seconds set in this example then it will stop on completing oiling and immediately restart trying to oil as it has already exceeded the target oil interval. 

Depending upon the programming skill of the library user there are different options to get going:

Limited programming experience:

Automated sketch builder

There is a folder under examples called OilerBuilder In here is a windows executable - OilerBuilder.exe. This allows a user to fill in details about the number (max 6) and types of motors to be used in the implementation. Pins can be assigned and features can be enabled. At the end it will auto generate an arduino sketch that can be used as the start point for an implementation.

Familiar with coding arduino projects

There are a number of example arduino sketches that show how to use the library to build different oiler solutions. The GitHub Wiki documents all the library calls.

Software developer

The src library has all the code that can be amended or extended. The library is designed to use interrupts to monitor pin state changes and timed events. The goal is that any library user can configure the library and start it going without any need for calling the library in a loop to ensure it is getting cpu time to keep the system running. As such the code has underlying functions to support PCI interrupt handling and system timers. The Oilerlibrary functional code uses these to manipulate c++ objects representing motors and the machine being oiled. The motor object is inherited to represent the different type of motor and these are manipulated using an internal state table.

To use the Oilerbuilder download the release and install it. Then run the Oilerbuilder.exe from the directory in which it is located.
