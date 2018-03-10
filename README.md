# D'Home-O'Tic

This project is a repository of various files and code written for a DIY Home
Automation project.

Please refer to each section of this README for a detailed description about
the various files and pieces of this project.

## Arduino Gate Controller

The arduino gate controller is based, for simplicity, on an Arduino UNO R3
board. It covers multiple features, described hereafter.

### Swing Gate State-Watcher

In order to automate the handling (opening and closing) of a house's Swing
Gate, it is necessary to know its current state. As such, the two parts of the
Gate are handled separately, with one Reed Switch for each door, which can
accurately report whether the door is closed or not.

#### Hardware Setup and Logic

Each Reed Switch has its `Ground` connected to the `Digital Ground Pin`, and
its `Normally Closed` connected to a `Digital Pin` of the user's choosing.

The chosen `Digital Pin` is then configured as an `INPUT_PULLUP`, with a `HIGH`
default state. When closing, the Reed Switch will then pull the current to
`Ground`, making a reading on the `Digital Pin` return `LOW`.

#### Switches Quirks and work-arounds

Through experiment, the Reed Switch may be observed to be slightly unreliable
when going from the `Closed` circuit state to the `Open` circuit state. I
personally noticed that when slowly approaching one part of the switch to
another, the read state changed a few times very quickly.

This observation led to the implementation of a mechanism that takes into
account an input from the Reed Switch only once it has been stable for a given
amount of time (configured to 100ms in my case).

In the code, this mechanism is called the state-change watching mechanism.

### Swing Gate activator

In order to automate and remotely control the Gate from a remote device, which
is not the Motorisation's command; it is necessary to have a way to control the
opening and closing of the Gate's doors through some kind of DIY hardware.

According to the informations I could get my hands on, a big chunk of gate
motorizations are relying on a simple electric current acting as the trigger to
actuate the motors, which often handle themselves Opening/Closing of the Gate
according to the last previous action (Thus inverting the movement).

This transmission of current can be induced by connecting a two-channel relay
to the electric current cables connected to the command module of the
Motorization. Using the relay, one can then allow the current to pass through,
simulating the activation of the Gate's motors by the command module.

In my case, these two-channels relays are powered by a 5V current, and can
handle up to 10A, and 30 VDC or 250 VAC.

#### Hardware Setup and Logic

Each two-channel relay has three inputs required, additionally to the
connection with the Motorization hardware:
 * `Ground pin`
 * `Power source`
 * `Input pin`

Of course, if you own a board with multiple relays on it, chances are that you
only need to connect the `Ground pin` and the `Power source` once. As the
Arduino UNO R3 provides a 5V power source pin, it is easy to handle a
two-channel relay with a 5V requirement.

Then the last pin, which is the `Input pin`, can be connected to any `Digital
Pin` on the arduino board. This pin can be used as an output, with a default
state of `HIGH`. Setting it to `LOW` will have for effect to activate the
two-channel relay, allowing current to bypass the motorization's command module
and activate the motors.

### Serial communication protocol

In order for the Arduino board to be integrated in a more complete Home
Automation system, it is necessary for it to communicate with a central brain.
In my case, this brain is a Raspberry-Pi 3.

Multiple methods are available to make an Arduino board communicate with a
R-Pi, including Radio/WiFi/Ethernet Arduino Shields and so on...

In my case, I chose to go simple, and have a simple communication with the R-Pi
through a serial channel.

#### Hardware Setup and Logic

On the Arduino board, the setup for serial is quite straight-forward: the
`Digital Pins` 0 and 1 are dedicated to Serial communication when directly
connected.

Beware, though, the power emitted by the Arduino board is 5v strong, while the
R-Pi only support 3.5V strong currents. Without a proper electrical level
converter between the two, there is a risk of frying the R-Pi, according to
reports regularly found on the web.

To solve this, I'm using a level-converter with a bi-directional bus for 5V to
3V. Please refer to tutorials on how to Hookup and Use a bi-directional Level
Converter.

#### Protocol definition

The protocol definitions can be found in the file gate\_protocol.h. The main
take-away of that file is that the protocol is made up of 1-byte messages,
where each bit represents one sensor/actuator state controlled by the Arduino
board.

##### Requests to the Arduino board

A status request can be made by sending a 0-value byte to the Arduino board, to
which it will answer with the complete set of states available (all sensors and
actuators).

In order to request the activation of an actuator, it is required to set the
bit associated to this actuator to 1, and the Arduino will execute the command.

##### Responses to requests

The Arduino will answer any request with the set of current states, setting the
bits of the sensors and actuator if they're in a non-normal state (For gate
sensors, Open is a non-normal state, and for the actuator, activated is a
non-normal state).

##### Summary

As for the information flow:
 - Arduino may notify the R-Pi arbitrarily of a state change through a byte
   with all bits set according to the sensors and actuators states (equivalent
   to a "Response" byte answering a "Request" byte which has all bits set)
 - R-pi may request a set of bits from the Arduino through a "Request" byte
 - Arduino may answer to R-pi through a "Response" byte.

Also, here is the definition of the various bit's meaning:
 - 1 << 0: Gate's left door sensor.
   0 means "Closed", 1 means "Opened"
 - 1 << 1: Gate's left door actuator.
   0 means "Not activating", 1 means "Activating"
 - 1 << 2: Gate's right door sensor.
   0 means "Closed", 1 means "Opened"
 - 1 << 3: Gate's right door actuator.
   0 means "Not activating", 1 means "Activating"
 - 1 << 4: Garage door sensor.
   0 means "Closed", 1 means "Opened"
 - 1 << 5: Garage door actuator.
   0 means "Not activating", 1 means "Activating"
 - 1 << 6: Guest door sensor.
   0 means "Closed", 1 means "Opened"
 - 1 << 7: Guest door actuator.
   0 means "Not activating", 1 means "Activating"
