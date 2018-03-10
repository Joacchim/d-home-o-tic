enum gwp_bits {
	GWP_GATE_LEFT_SENSOR	  = 0,
	GWP_GATE_LEFT_ACTUATOR	= 1,
	GWP_GATE_RIGHT_SENSOR	  = 2,
	GWP_GATE_RIGHT_ACTUATOR	= 3,
	GWP_GARAGE_SENSOR		    = 4,
	GWP_GARAGE_ACTUATOR		  = 5,
	GWP_GUEST_SENSOR		    = 6,
	GWP_GUEST_ACTUATOR		  = 7,
};

enum sensor_state {
	SENSOR_CLOSED	=	0,
	SENSOR_OPEN	=	1,
};

enum actuator_state {
	ACTUATOR_INACTIVE	=	0,
	ACTUATOR_ACTIVE		=	1,
};

#define SET_BIT(byte, bit, value) ((byte & ~(1U << bit)) | (value << bit))
#define GET_BIT(byte, bit) (byte & (1u << bit))
