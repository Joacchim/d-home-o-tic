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
