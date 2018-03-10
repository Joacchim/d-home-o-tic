#include "gate_protocol.h"

#define SERIAL_BPS_RATE         9600
#define UNSIGNED_LONG_INT_MAX   (2^32 - 1)

/*
 * DIGITAL PINs association definitions
 */
#define PIN_RIGHT_DOOR          4
#define PIN_RIGHT_DOOR_CONTACT  5
#define PIN_LEFT_DOOR           6
#define PIN_LEFT_DOOR_CONTACT   7

#define GATE_ACTIONATE_TIME_MS      1000
#define GATE_STATE_MIN_DURATION_MS  100

enum door_state {
  DOOR_STATE_OPEN =     LOW,
  DOOR_STATE_CLOSED =   HIGH,
};

enum door_action {
  NONE = 0,
  TRIGGER = 1,
};

struct door_desc {
  const int         pin;
  const int         contact_pin;
  bool              initialized;
  enum door_state   state;
  enum door_state   new_state;
  bool              state_changing;
  unsigned long int state_changed_on;
  enum door_action  action;
  unsigned long int action_started_on;
  int               sensor_bit;
  int               actuator_bit;
};

struct door_desc doors[] = {
  {
    PIN_RIGHT_DOOR,  PIN_RIGHT_DOOR_CONTACT, false,
    DOOR_STATE_CLOSED, DOOR_STATE_CLOSED, false, 0ul,
    NONE, 0ul,
    GWP_GATE_LEFT_SENSOR, GWP_GATE_LEFT_ACTUATOR
  },
  {
    PIN_LEFT_DOOR, PIN_LEFT_DOOR_CONTACT, false,
    DOOR_STATE_CLOSED, DOOR_STATE_CLOSED, false, 0ul,
    NONE, 0ul,
    GWP_GATE_RIGHT_SENSOR, GWP_GATE_RIGHT_ACTUATOR
  },
};

/*
 * This function computes a duration, while trying to take into account the fact
 * that the millis() function provides the number of ms since the start of program
 * into 32bits, and will thus overflow (wrap around through 0) about every 50 days.
 *
 * As such, thus function cannot be used to compute a duration longer than ~40 days
 * (safety margin), and should be used accordingly.
 */
unsigned long int durationSince(unsigned long int origin_ms) {
  unsigned long int cur = millis();
  unsigned long int duration = 0ul;
  // Handle the overflow around every 50days (Documented in Arduino reference)
  if (cur < origin_ms)
    duration = UNSIGNED_LONG_INT_MAX - origin_ms + cur;
  else
    duration = cur - origin_ms;
  return duration;
}

/*
 * Utility function to map an enum door_state
 * to a bit value in the Serial protocol
 */
int sensor_to_bit(enum door_state state) {
  const int bits[] = {
    [DOOR_STATE_OPEN]   = 1,
    [DOOR_STATE_CLOSED] = 0,
  };
  return bits[state];
}

/*
 * Utility function to map an enum door_action
 * to a bit value in the Serial protocol
 */
int actuator_to_bit(enum door_action action) {
  const int bits[] = {
    [NONE] = 0,
    [TRIGGER] = 1,
  };
  return bits[action];
}

/*
 * Iterate on the doors, and set the bits requested by the request
 * input byte into a byte originally set to 0.
 * The byte computed this way is then sent over the Serial
 * communication channel.
 */
void notify_state(uint8_t request) {
  uint8_t data = 0;
  for (int i=0; i < sizeof(doors)/sizeof(*doors) ; i++) {
    if (bitRead(request, doors[i].sensor_bit))
      data = SET_BIT(data, doors[i].sensor_bit, sensor_to_bit(doors[i].state));
    if (bitRead(request, doors[i].actuator_bit))
      data = SET_BIT(data, doors[i].actuator_bit, actuator_to_bit(doors[i].action));
  }
  Serial.write(data);
}

/*
 * Simulate a button pushed for PUSH_TIME_MS milliseconds,
 * that sends power through the Door's motors.
 */
void doorStartActionate(struct door_desc *door) {
  digitalWrite(door->pin, LOW);
  door->action = TRIGGER;
  door->action_started_on = millis();
}

/*
 * Checks whether enough time passed to stop "pushing" the button
 * to change the Gate's door state (trigger opening/closing)
 */
bool doorEndActionate(struct door_desc *door) {
  if (door->action != NONE
      && durationSince(door->action_started_on) >= GATE_ACTIONATE_TIME_MS) {
    digitalWrite(door->pin, HIGH);
    door->action = NONE;
    /*
     * Stopped the activation of the actuator
     * -> Notify state change to R-Pi
     */
    notify_state(0xF);
  }
}

/*
 * Convert the boolean returned by digitalRead into the door_state enum
 */
enum door_state toDoorState(bool value) {
  const enum door_state states[] = {
    // Current pulled to ground -> circuit is "closed", so is the door
    [LOW] = DOOR_STATE_CLOSED,
    // Current not pulled to ground -> circuit is "open"; so is the door
    [HIGH] = DOOR_STATE_OPEN,
  };
  return states[value];
}

/*
 * Get the door's state, update it, and send a message on
 * the communication channel if the state changed since the
 * last call of this function
 */
void doorRefreshState(struct door_desc *door) {
  enum door_state new_state = toDoorState(digitalRead(door->contact_pin));

  /*
   * If a state change was just observed, start the state-change watching mechanism.
   * This aims to ensure that the new state is stable enough to communicate this
   * information to the R-Pi
   */
  if (door->initialized && new_state != door->state) {
    door->state_changing = true;
    door->state_changed_on = millis();
    door->new_state = new_state;
  }

  /*
   * Handle the state-change watching mechanism.
   * This requires two actions:
   * 1. Maintain the measurement of how long the state was stable. This relies on
   *    the durationSince function.
   * 2. Stop the state-change watching mechanism and Send a notification to R-Pi
   *    if the state has been stable for long enough
   *
   *
   * The state-change watching mechanism is necessary for the following reason:
   *   When opening slowly, the Reed Switches tend to become flaky around the
   *   magnetic field's limit of effectiveness. Due to this, they quickly alternate
   *   between open and closed, making us uncertain of the actual state. As such,
   *   let's ensure that any one state must remain for more than a hard-coded length
   *   of time before being taken into account for our door logic.
   */
  if (door->state_changing) {
    /*
     * Send a notification to R-pi only when the door changed state for long enough
     * for the information to be trustworthy.
     *
     * This accounts for the state to have changed more than
     * GATE_STATE_MIN_DURATION_MS milliseconds.
     */
    if (durationSince(door->state_changed_on) > GATE_STATE_MIN_DURATION_MS) {
      door->state_changing = false;
      door->state = door->new_state;

      /*
       * Send state change notification to R-Pi
       */
      notify_state(0xF);
    }
  }
}

/*
 * Setup a door's internal state:
 *  - Define the right mode for each pin
 *  - Define the default status of the output pin
 *  - Get the initial door state
 *  - Set the initialized flag for the door
 */
void doorSetup(struct door_desc *door) {
  pinMode(door->pin, OUTPUT);
  digitalWrite(door->pin, HIGH);
  pinMode(door->contact_pin, INPUT_PULLUP);
  digitalWrite(door->contact_pin, HIGH);

  doorRefreshState(door);
  door->initialized = true;
}

/*
 * Initialize the state of all doors,
 * and setup the communication channel.
 *
 * This function is only called once at the startup
 * of the Arduino board
 */
void setup() {
  /*
   * Setup communication channel with RPi
   */
  Serial.begin(SERIAL_BPS_RATE);

  /*
   * Initialize and retrieve the initial state of the doors
   */
  for (int i=0; i < sizeof(doors)/sizeof(*doors) ; ++i) {
    doorSetup(&doors[i]);
  }
}

/*
 * Refresh the door's states, and push state changes
 * to the controller RPi.
 *
 * This function is repeatedly excecuted by the core
 * of the Arduino board.
 */
void loop() {
  for (int i=0; i < sizeof(doors)/sizeof(*doors) ; ++i) {
    doorRefreshState(&doors[i]);
    doorEndActionate(&doors[i]);
  }
}

/*
 * Listen for the serial Events, as requests shall come through this channel.
 */
void serialEvent() {
  while (Serial.available()) {
    uint8_t request = (uint8_t)Serial.read();
    // First, activate any requested actuator supported
    for (int i=0; i < sizeof(doors)/sizeof(*doors) ; ++i) {
      if (GET_BIT(request, doors[i].actuator_bit)) {
        doorStartActionate(&doors[i]);
      }
    }
    // Finally, notify of the current state
    notify_state(0xF);
  }
}
