/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef MAGIC_H
#define MAGIC_H
#include <stdint.h>
#include <string.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* The version *we* support */
#define RTDE_SUPPORTED_PROTOCOL_VERSION 2
enum RTDE_PACKAGE_TYPE {
	RTDE_REQUEST_PROTOCOL_VERSION	= 86,  // 'V'
	RTDE_GET_URCONTROL_VERSION	= 118, // 'v'
	RTDE_TEXT_MESSAGE		= 77,  // 'M'
	RTDE_DATA_PACKAGE		= 85,  // 'U'
	RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS = 79,// 'O'
	RTDE_CONTROL_PACKAGE_SETUP_INPUTS  = 73,// 'I'
	RTDE_CONTROL_PACKAGE_START	= 83, // 'S'
	RTDE_CONTROL_PACKAGE_PAUSE	= 80, // 'P'
};

enum  RTDE_MESSAGE_TYPES {
	RTDE_EXCEPTION_MESSAGE = 0,
	RTDE_ERROR_MESSAGE,
	RTDE_WARNING_MESSAGE,
	RTDE_INFO_MESSAGE,
};

enum RTDE_DATA_TYPE {
	BOOL = 0,
	UINT8,
	UINT32,
	UINT64,
	INT32,
	DOUBLE, 	// IEEE 754 floating point 	64
	VECTOR3D, 	// 3xDOUBLE
	VECTOR6D, 	// 6xDOUBLE
	VECTOR6INT32, 	//6xINT32
	VECTOR6UINT32, 	//6xUINT32
	STRING, 	//ASCII char array
	IN_USE,
	NOT_FOUND,
};

static inline enum RTDE_DATA_TYPE data_name_to_type(const char *name, size_t sz)
{
	if (!name)
		return NOT_FOUND;
	if (strncmp(name, "BOOL", sz) == 0)
		return BOOL;
	else if (strncmp(name, "UINT8", sz) == 0)
		return UINT8;
	else if (strncmp(name, "UINT32", sz) == 0)
		return UINT32;
	else if (strncmp(name, "UINT64", sz) == 0)
		return UINT64;
	else if (strncmp(name, "INT32", sz) == 0)
		return INT32;
	else if (strncmp(name, "DOUBLE", sz) == 0)
		return DOUBLE;
	else if (strncmp(name, "VECTOR3D", sz) == 0)
		return VECTOR3D;
	else if (strncmp(name, "VECTOR6D", sz) == 0)
		return VECTOR6D;
	else if (strncmp(name, "VECTOR6INT32", sz) == 0)
		return VECTOR6INT32;
	else if (strncmp(name, "VECTOR6UINT32", sz) == 0)
		return VECTOR6UINT32;
	else if (strncmp(name, "STRING", sz) == 0)
		return STRING;
	else if (strncmp(name, "IN_USE", sz) == 0)
		return IN_USE;
	else
		return NOT_FOUND;
}

static uint8_t rtde_datasize[NOT_FOUND+1] = {
	1,	/* BOOL */
	1,	/* UINT8, */
	4,      /* UINT32, */
	8,	/* UINT64, */
	4,	/* INT32, */
	8,	/* DOUBLE - IEEE 754 floating point */
	24,	/* VECTOR3D, 	// 3xDOUBLE */
	48,	/* VECTOR6D, 	// 6xDOUBLE */
	24,	/* VECTOR6INT32, 	//6xINT32 */
	24,	/* VECTOR6UINT32, 	//6xUINT32 */
	1,	/* STRING, 1 * strlen 	//ASCII char array */
	0,	/* in_use */
	0,	/* not found */
};
static inline int type_to_size(enum RTDE_DATA_TYPE type) { return rtde_datasize[type]; }

struct rtde_data_names {
	char name[64];
	enum RTDE_DATA_TYPE type;
};


static struct rtde_data_names rtde_input_names[] = {
	{ "speed_slider_mask", 	UINT32},	/* 0 = don't change speed slider with this input
						 *  1 = use speed_slider_fraction to set speed slider value
						 */
	{ "speed_slider_fraction",		DOUBLE }, // new speed slider value
	{ "standard_digital_output_mask", 	UINT8  }, // Standard digital output bit mask*
	{ "standard_digital_output",		UINT8  }, // Standard digital outputs
	{ "configurable_digital_output_mask", 	UINT8  }, // Configurable digital output bit mask*
	{ "configurable_digital_output", 	UINT8  }, // Configurable digital outputs
	{ "tool_digital_output_mask",		UINT8  }, /* Tool digital outputs mask*
							   * Bits 0-1: mask, remaining bits are reserved for future use
							   */
	{ "tool_digital_output",		UINT8  },  /* Tool digital outputs
							   * Bits 0-1: output state, remaining bits are reserved for future use
							   */
	{ "standard_analog_output_mask", 	UINT8  }, /* Standard analog output mask
							   * Bits 0-1: standard_analog_output_0 | standard_analog_output_1
							   */
	{ "standard_analog_output_type", 	UINT8  }, /* Output domain {0=current[A], 1=voltage[V]}
							   * Bits 0-1: standard_analog_output_0 | standard_analog_output_1
							   */
	{ "standard_analog_output_0",		DOUBLE }, // Standard analog output 0 (ratio) [0..1]
	{ "standard_analog_output_1",		DOUBLE }, // Standard analog output 1 (ratio) [0..1]

	/* General purpose bits
	 *
	 * This range of the boolean input registers is reserved for
	 * FieldBus/PLC interface usage.
	 */
	{ "input_bit_registers0_to_31", 	UINT32 },

	/* General purpose bits
	 *
	 * This range of the boolean input registers is reserved for
	 * FieldBus/PLC interface usage.
	 */
	{ "input_bit_registers32_to_63", 	UINT32 },


	/* ==============================================================
	 * FIXME
	 *
	 * Must add the remaining regs here! (these are not currently
	 * not valid.
	 *==============================================================*/

	/*
	 * 64 general purpose bits
	 *
	 * X: [64..127] - The upper range of the boolean input registers
	 *		  can be used by external RTDE clients (i.e URCAPS).
	 */
	{ "input_bit_register_X",		BOOL },

	/*
	 * 48 general purpose integer registers
	 * X: [0..23] - The lower range of the integer input registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the integer input registers can be used by external RTDE clients (i.e URCAPS).
	 *
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{ "input_int_register_0", 	INT32 },
	{ "input_int_register_1", 	INT32 },
	{ "input_int_register_2", 	INT32 },
	{ "input_int_register_3", 	INT32 },
	{ "input_int_register_4", 	INT32 },
	{ "input_int_register_5", 	INT32 },
	{ "input_int_register_6", 	INT32 },
	{ "input_int_register_7", 	INT32 },
	{ "input_int_register_8", 	INT32 },
	{ "input_int_register_9", 	INT32 },
	{ "input_int_register_10", 	INT32 },
	{ "input_int_register_11", 	INT32 },
	{ "input_int_register_12", 	INT32 },
	{ "input_int_register_13", 	INT32 },
	{ "input_int_register_14", 	INT32 },
	{ "input_int_register_15", 	INT32 },
	{ "input_int_register_16", 	INT32 },
	{ "input_int_register_17", 	INT32 },
	{ "input_int_register_18", 	INT32 },
	{ "input_int_register_19", 	INT32 },
	{ "input_int_register_20", 	INT32 },
	{ "input_int_register_21", 	INT32 },
	{ "input_int_register_22", 	INT32 },
	{ "input_int_register_23", 	INT32 },
	{ "input_int_register_24", 	INT32 },
	{ "input_int_register_25", 	INT32 },
	{ "input_int_register_26", 	INT32 },
	{ "input_int_register_27", 	INT32 },
	{ "input_int_register_28", 	INT32 },
	{ "input_int_register_29", 	INT32 },
	{ "input_int_register_30", 	INT32 },
	{ "input_int_register_31", 	INT32 },
	{ "input_int_register_32", 	INT32 },
	{ "input_int_register_33", 	INT32 },
	{ "input_int_register_34", 	INT32 },
	{ "input_int_register_35", 	INT32 },
	{ "input_int_register_36", 	INT32 },
	{ "input_int_register_37", 	INT32 },
	{ "input_int_register_38", 	INT32 },
	{ "input_int_register_39", 	INT32 },
	{ "input_int_register_40", 	INT32 },
	{ "input_int_register_41", 	INT32 },
	{ "input_int_register_42", 	INT32 },
	{ "input_int_register_43", 	INT32 },
	{ "input_int_register_44", 	INT32 },
	{ "input_int_register_45", 	INT32 },
	{ "input_int_register_46", 	INT32 },
	{ "input_int_register_47", 	INT32 },

	/*
	 * 48 general purpose double registers
	 *
	 * X: [0..23] - The lower range of the double input registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the double input registers can be used by external RTDE clients (i.e URCAPS).
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{ "input_double_register_0", 	DOUBLE },
	{ "input_double_register_1", 	DOUBLE },
	{ "input_double_register_2", 	DOUBLE },
	{ "input_double_register_3", 	DOUBLE },
	{ "input_double_register_4", 	DOUBLE },
	{ "input_double_register_5", 	DOUBLE },
	{ "input_double_register_6", 	DOUBLE },
	{ "input_double_register_7", 	DOUBLE },
	{ "input_double_register_8", 	DOUBLE },
	{ "input_double_register_9", 	DOUBLE },
	{ "input_double_register_10", 	DOUBLE },
	{ "input_double_register_11", 	DOUBLE },
	{ "input_double_register_12", 	DOUBLE },
	{ "input_double_register_13", 	DOUBLE },
	{ "input_double_register_14", 	DOUBLE },
	{ "input_double_register_15", 	DOUBLE },
	{ "input_double_register_16", 	DOUBLE },
	{ "input_double_register_17", 	DOUBLE },
	{ "input_double_register_18", 	DOUBLE },
	{ "input_double_register_19", 	DOUBLE },
	{ "input_double_register_20", 	DOUBLE },
	{ "input_double_register_21", 	DOUBLE },
	{ "input_double_register_22", 	DOUBLE },
	{ "input_double_register_23", 	DOUBLE },
	{ "input_double_register_24", 	DOUBLE },
	{ "input_double_register_25", 	DOUBLE },
	{ "input_double_register_26", 	DOUBLE },
	{ "input_double_register_27", 	DOUBLE },
	{ "input_double_register_28", 	DOUBLE },
	{ "input_double_register_29", 	DOUBLE },
	{ "input_double_register_30", 	DOUBLE },
	{ "input_double_register_31", 	DOUBLE },
	{ "input_double_register_32", 	DOUBLE },
	{ "input_double_register_33", 	DOUBLE },
	{ "input_double_register_34", 	DOUBLE },
	{ "input_double_register_35", 	DOUBLE },
	{ "input_double_register_36", 	DOUBLE },
	{ "input_double_register_37", 	DOUBLE },
	{ "input_double_register_38", 	DOUBLE },
	{ "input_double_register_39", 	DOUBLE },
	{ "input_double_register_40", 	DOUBLE },
	{ "input_double_register_41", 	DOUBLE },
	{ "input_double_register_42", 	DOUBLE },
	{ "input_double_register_43", 	DOUBLE },
	{ "input_double_register_44", 	DOUBLE },
	{ "input_double_register_45", 	DOUBLE },
	{ "input_double_register_46", 	DOUBLE },
	{ "input_double_register_47", 	DOUBLE },
};

static inline int input_name_to_idx(const char *name) {
	for (size_t i = 0; i < sizeof(rtde_input_names)/sizeof(struct rtde_data_names); i++) {
		if (strcmp(name, rtde_input_names[i].name) == 0)
			return i;
	}
	return -1;
}

static inline enum RTDE_DATA_TYPE input_name_to_type(const char *name) {
	int idx = input_name_to_idx(name);
	if (idx < 0)
		return NOT_FOUND;
	return rtde_input_names[idx].type;
}

static struct rtde_data_names rtde_output_names[] = {

	{ "timestamp",			DOUBLE},   /* Time elapsed since the controller was started [s] */
	{ "target_q",			VECTOR6D}, 	/* Target joint positions */
	{ "target_qd",			VECTOR6D}, 	/* Target joint velocities */
	{ "target_qdd",			VECTOR6D}, 	/* Target joint accelerations */
	{ "target_current",		VECTOR6D}, 	/* Target joint currents */
	{ "target_moment",		VECTOR6D}, 	/* Target joint moments (torques) */
	{ "actual_q",			VECTOR6D}, 	/* Actual joint positions */
	{ "actual_qd",			VECTOR6D}, 	/* Actual joint velocities */
	{ "actual_current",		VECTOR6D}, 	/* Actual joint currents */
	{ "joint_control_output",	VECTOR6D}, 	/* Joint control currents */
	{ "actual_TCP_pose",		VECTOR6D}, 	/* Actual Cartesian coordinates of the tool: (x,y,z,rx,ry,rz), where rx, ry and rz is a rotation vector representation of the tool orientation */
	{ "actual_TCP_speed",		VECTOR6D}, 	/* Actual speed of the tool given in Cartesian coordinates */
	{ "actual_TCP_force",		VECTOR6D}, 	/* Generalized forces in the TCP */
	{ "target_TCP_pose",		VECTOR6D}, 	/* Target Cartesian coordinates of the tool: (x,y,z,rx,ry,rz), where rx, ry and rz is a rotation vector representation of the tool orientation */
	{ "target_TCP_speed",		VECTOR6D}, 	/* Target speed of the tool given in Cartesian coordinates */
	{ "actual_digital_input_bits", 	UINT64}, 	/* Current state of the digital inputs. 0-7: Standard, 8-15: Configurable, 16-17: Tool */
	{ "joint_temperatures", 	VECTOR6D}, 	/* Temperature of each joint in degrees Celsius */
	{ "actual_execution_time", 	DOUBLE}, 	/* Controller real-time thread execution time */
	{ "robot_mode",			INT32}, 	/* Robot mode Please", see Remote Control Via TCP/IP - 16496 */
	{ "joint_mode",			VECTOR6INT32}, 	/* Joint control modes Please", see Remote Control Via TCP/IP - 16496 */
	{ "safety_mode",		INT32}, 	/* Safety mode Please", see Remote Control Via TCP/IP - 16496 */
	{ "safety_status",		INT32}, 	/* Safety ststus 	3.10.0/5.4.0 */
	{ "actual_tool_accelerometer", 	VECTOR3D}, 	/* Tool x, y and z accelerometer values */
	{ "speed_scaling",		DOUBLE}, 	/* Speed scaling of the trajectory limiter */
	{ "target_speed_fraction", 	DOUBLE}, 	/* Target speed fraction */
	{ "actual_momentum",		DOUBLE}, 	/* Norm of Cartesian linear momentum */
	{ "actual_main_voltage", 	DOUBLE}, 	/* Safety Control Board: Main voltage */
	{ "actual_robot_voltage", 	DOUBLE}, 	/* Safety Control Board: Robot voltage (48V) */
	{ "actual_robot_current", 	DOUBLE}, 	/* Safety Control Board: Robot current */
	{ "actual_joint_voltage", 	VECTOR6D}, 	/* Actual joint voltages */
	{ "actual_digital_output_bits", UINT64}, 	/* Current state of the digital outputs. 0-7: Standard, 8-15: Configurable, 16-17: Tool */
	{ "runtime_state",		UINT32}, 	/* Program state */
	{ "elbow_position",		VECTOR3D}, 	/* Position of robot elbow in Cartesian Base Coordinates 	3.5.0/5.0.0 */
	{ "elbow_velocity",		VECTOR3D}, 	/* Velocity of robot elbow in Cartesian Base Coordinates 	3.5.0/5.0.0 */
	{ "robot_status_bits",		UINT32}, 	/* Bits 0-3: Is power on | Is program running | Is teach button pressed | Is power button pressed */
	{ "safety_status_bits", 	UINT32}, 	/* Bits 0-10: Is normal mode | Is reduced mode | | Is protective stopped | Is recovery mode | Is safeguard stopped | Is system emergency stopped | Is robot emergency stopped | Is emergency stopped | Is violation | Is fault | Is stopped due to safety */
	{ "analog_io_types",		UINT32},	/* Bits 0-3: analog input 0 | analog input 1 | analog output 0 | analog output 1, {0=current[A], 1=voltage[V]} */
	{ "standard_analog_input0", 	DOUBLE}, 	/* Standard analog input 0 [A or V]  */
	{ "standard_analog_input1", 	DOUBLE}, 	/* Standard analog input 1 [A or V] */
	{ "standard_analog_output0", 	DOUBLE}, 	/* Standard analog output 0 [A or V] */
	{ "standard_analog_output1", 	DOUBLE}, 	/* Standard analog output 1 [A or V] */
	{ "io_current", 	DOUBLE}, 	/* I/O current [A] */
	{ "euromap67_input_bits", 	UINT32}, 	 /* Euromap67 input bits */
	{ "euromap67_output_bits", 	UINT32}, 	 /* Euromap67 output bits */
	{ "euromap67_24V_voltage", 	DOUBLE}, 	/* Euromap 24V voltage [V] */
	{ "euromap67_24V_current", 	DOUBLE}, 	/* Euromap 24V current [A] */
	{ "tool_mode", 	UINT32}, 	/* Tool mode Please see Remote Control Via TCP/IP - 16496 */
	{ "tool_analog_input_types", 	UINT32}, 	 /* Output domain {0=current[A], 1=voltage[V]} Bits 0-1: tool_analog_input_0 | tool_analog_input_1 */
	{ "tool_analog_input0", 	DOUBLE}, 	/* Tool analog input 0 [A or V] */
	{ "tool_analog_input1", 	DOUBLE}, 	/* Tool analog input 1 [A or V] */
	{ "tool_output_voltage", 	INT32}, 	/* Tool output voltage [V] */
	{ "tool_output_current", 	DOUBLE}, 	/* Tool current [A] */
	{ "tool_temperature", 	DOUBLE}, 	/* Tool temperature in degrees Celsius */
	{ "tcp_force_scalar", 	DOUBLE}, 	/* TCP force scalar [N] */
	{ "output_bit_registers0_to_31", 	UINT32}, 	/* General purpose bits */
	{ "output_bit_registers32_to_63", 	UINT32}, 	/* General purpose bits */



	/* 64 general purpose bits
	 * X: [64..127] - The upper range of the boolean output
	 * registers can be used by external RTDE clients (i.e URCAPS).
	 */
	{ "output_bit_register_X", 	BOOL},

	/* 48 general purpose integer registers
	 * X: [0..23] - The lower range of the integer output registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the integer output registers can be used by external RTDE clients (i.e URCAPS).
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{ "output_int_register_0", 	INT32},
	{ "output_int_register_1", 	INT32},
	{ "output_int_register_2", 	INT32},
	{ "output_int_register_3", 	INT32},
	{ "output_int_register_4", 	INT32},
	{ "output_int_register_5", 	INT32},
	{ "output_int_register_6", 	INT32},
	{ "output_int_register_7", 	INT32},
	{ "output_int_register_8", 	INT32},
	{ "output_int_register_9", 	INT32},
	{ "output_int_register_10", 	INT32},
	{ "output_int_register_11", 	INT32},
	{ "output_int_register_12", 	INT32},
	{ "output_int_register_13", 	INT32},
	{ "output_int_register_14", 	INT32},
	{ "output_int_register_15", 	INT32},
	{ "output_int_register_16", 	INT32},
	{ "output_int_register_17", 	INT32},
	{ "output_int_register_18", 	INT32},
	{ "output_int_register_19", 	INT32},
	{ "output_int_register_20", 	INT32},
	{ "output_int_register_21", 	INT32},
	{ "output_int_register_22", 	INT32},
	{ "output_int_register_23", 	INT32},
	{ "output_int_register_24", 	INT32},
	{ "output_int_register_25", 	INT32},
	{ "output_int_register_26", 	INT32},
	{ "output_int_register_27", 	INT32},
	{ "output_int_register_28", 	INT32},
	{ "output_int_register_29", 	INT32},
	{ "output_int_register_30", 	INT32},
	{ "output_int_register_31", 	INT32},
	{ "output_int_register_32", 	INT32},
	{ "output_int_register_33", 	INT32},
	{ "output_int_register_34", 	INT32},
	{ "output_int_register_35", 	INT32},
	{ "output_int_register_36", 	INT32},
	{ "output_int_register_37", 	INT32},
	{ "output_int_register_38", 	INT32},
	{ "output_int_register_39", 	INT32},
	{ "output_int_register_40", 	INT32},
	{ "output_int_register_41", 	INT32},
	{ "output_int_register_42", 	INT32},
	{ "output_int_register_43", 	INT32},
	{ "output_int_register_44", 	INT32},
	{ "output_int_register_45", 	INT32},
	{ "output_int_register_46", 	INT32},
	{ "output_int_register_47", 	INT32},

	/* 48 general purpose double registers
	 * X: [0..23] - The lower range of the double output registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the double output registers can be used by external RTDE clients (i.e URCAPS).
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{ "output_double_register_0", 	DOUBLE},
	{ "output_double_register_1", 	DOUBLE},
	{ "output_double_register_2", 	DOUBLE},
	{ "output_double_register_3", 	DOUBLE},
	{ "output_double_register_4", 	DOUBLE},
	{ "output_double_register_5", 	DOUBLE},
	{ "output_double_register_6", 	DOUBLE},
	{ "output_double_register_7", 	DOUBLE},
	{ "output_double_register_8", 	DOUBLE},
	{ "output_double_register_9", 	DOUBLE},
	{ "output_double_register_10", 	DOUBLE},
	{ "output_double_register_11", 	DOUBLE},
	{ "output_double_register_12", 	DOUBLE},
	{ "output_double_register_13", 	DOUBLE},
	{ "output_double_register_14", 	DOUBLE},
	{ "output_double_register_15", 	DOUBLE},
	{ "output_double_register_16", 	DOUBLE},
	{ "output_double_register_17", 	DOUBLE},
	{ "output_double_register_18", 	DOUBLE},
	{ "output_double_register_19", 	DOUBLE},
	{ "output_double_register_20", 	DOUBLE},
	{ "output_double_register_21", 	DOUBLE},
	{ "output_double_register_22", 	DOUBLE},
	{ "output_double_register_23", 	DOUBLE},
	{ "output_double_register_24", 	DOUBLE},
	{ "output_double_register_25", 	DOUBLE},
	{ "output_double_register_26", 	DOUBLE},
	{ "output_double_register_27", 	DOUBLE},
	{ "output_double_register_28", 	DOUBLE},
	{ "output_double_register_29", 	DOUBLE},
	{ "output_double_register_30", 	DOUBLE},
	{ "output_double_register_31", 	DOUBLE},
	{ "output_double_register_32", 	DOUBLE},
	{ "output_double_register_33", 	DOUBLE},
	{ "output_double_register_34", 	DOUBLE},
	{ "output_double_register_35", 	DOUBLE},
	{ "output_double_register_36", 	DOUBLE},
	{ "output_double_register_37", 	DOUBLE},
	{ "output_double_register_38", 	DOUBLE},
	{ "output_double_register_39", 	DOUBLE},
	{ "output_double_register_40", 	DOUBLE},
	{ "output_double_register_41", 	DOUBLE},
	{ "output_double_register_42", 	DOUBLE},
	{ "output_double_register_43", 	DOUBLE},
	{ "output_double_register_44", 	DOUBLE},
	{ "output_double_register_45", 	DOUBLE},
	{ "output_double_register_46", 	DOUBLE},
	{ "output_double_register_47", 	DOUBLE},

	/* General purpose bits
	 * This range of the boolean output registers is reserved for FieldBus/PLC interface usage.
	 * 3.4.0
	 */
	{ "input_bit_registers0_to_31", 	UINT32}, /*  */

	/*  General purpose bits
	 * This range of the boolean output registers is reserved for FieldBus/PLC interface usage.
	 * 3.4.0
	 */
	{ "input_bit_registers32_to_63", 	UINT32}, /*  */

	/* 64 general purpose bits
	 * X: [64..127] - The upper range of the boolean output registers can be used by external RTDE clients (i.e URCAPS).
	 * 3.9.0 / 5.3.0
	 */
	{ "input_bit_register_X", 	BOOL},

	/* 48 general purpose integer registers
	 * X: [0..23] - The lower range of the integer input registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the integer input registers can be used by external RTDE clients (i.e URCAPS).
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{ "input_int_register_0", INT32},
	{ "input_int_register_1", INT32},
	{ "input_int_register_2", INT32},
	{ "input_int_register_3", INT32},
	{ "input_int_register_4", INT32},
	{ "input_int_register_5", INT32},
	{ "input_int_register_6", INT32},
	{ "input_int_register_7", INT32},
	{ "input_int_register_8", INT32},
	{ "input_int_register_9", INT32},
	{ "input_int_register_10", INT32},
	{ "input_int_register_11", INT32},
	{ "input_int_register_12", INT32},
	{ "input_int_register_13", INT32},
	{ "input_int_register_14", INT32},
	{ "input_int_register_15", INT32},
	{ "input_int_register_16", INT32},
	{ "input_int_register_17", INT32},
	{ "input_int_register_18", INT32},
	{ "input_int_register_19", INT32},
	{ "input_int_register_20", INT32},
	{ "input_int_register_21", INT32},
	{ "input_int_register_22", INT32},
	{ "input_int_register_23", INT32},
	{ "input_int_register_24", INT32},
	{ "input_int_register_25", INT32},
	{ "input_int_register_26", INT32},
	{ "input_int_register_27", INT32},
	{ "input_int_register_28", INT32},
	{ "input_int_register_29", INT32},
	{ "input_int_register_30", INT32},
	{ "input_int_register_31", INT32},
	{ "input_int_register_32", INT32},
	{ "input_int_register_33", INT32},
	{ "input_int_register_34", INT32},
	{ "input_int_register_35", INT32},
	{ "input_int_register_36", INT32},
	{ "input_int_register_37", INT32},
	{ "input_int_register_38", INT32},
	{ "input_int_register_39", INT32},
	{ "input_int_register_40", INT32},
	{ "input_int_register_41", INT32},
	{ "input_int_register_42", INT32},
	{ "input_int_register_43", INT32},
	{ "input_int_register_44", INT32},
	{ "input_int_register_45", INT32},
	{ "input_int_register_46", INT32},
	{ "input_int_register_47", INT32},

	/* 48 general purpose double registers
	 * 1: [0..23] - The lower range of the double input registers is reserved for FieldBus/PLC interface usage.
	 * X: [24..47] - The upper range of the double input registers can be used by external RTDE clients (i.e URCAPS).
	 * [0..23] 3.4.0
	 * [24..47] 3.9.0 / 5.3.0
	 */
	{"input_double_register_0", DOUBLE},
	{"input_double_register_1", DOUBLE},
	{"input_double_register_2", DOUBLE},
	{"input_double_register_3", DOUBLE},
	{"input_double_register_4", DOUBLE},
	{"input_double_register_5", DOUBLE},
	{"input_double_register_6", DOUBLE},
	{"input_double_register_7", DOUBLE},
	{"input_double_register_8", DOUBLE},
	{"input_double_register_9", DOUBLE},
	{"input_double_register_10", DOUBLE},
	{"input_double_register_11", DOUBLE},
	{"input_double_register_12", DOUBLE},
	{"input_double_register_13", DOUBLE},
	{"input_double_register_14", DOUBLE},
	{"input_double_register_15", DOUBLE},
	{"input_double_register_16", DOUBLE},
	{"input_double_register_17", DOUBLE},
	{"input_double_register_18", DOUBLE},
	{"input_double_register_19", DOUBLE},
	{"input_double_register_20", DOUBLE},
	{"input_double_register_21", DOUBLE},
	{"input_double_register_22", DOUBLE},
	{"input_double_register_23", DOUBLE},
	{"input_double_register_24", DOUBLE},
	{"input_double_register_25", DOUBLE},
	{"input_double_register_26", DOUBLE},
	{"input_double_register_27", DOUBLE},
	{"input_double_register_28", DOUBLE},
	{"input_double_register_29", DOUBLE},
	{"input_double_register_30", DOUBLE},
	{"input_double_register_31", DOUBLE},
	{"input_double_register_32", DOUBLE},
	{"input_double_register_33", DOUBLE},
	{"input_double_register_34", DOUBLE},
	{"input_double_register_35", DOUBLE},
	{"input_double_register_36", DOUBLE},
	{"input_double_register_37", DOUBLE},
	{"input_double_register_38", DOUBLE},
	{"input_double_register_39", DOUBLE},
	{"input_double_register_40", DOUBLE},
	{"input_double_register_41", DOUBLE},
	{"input_double_register_42", DOUBLE},
	{"input_double_register_43", DOUBLE},
	{"input_double_register_44", DOUBLE},
	{"input_double_register_45", DOUBLE},
	{"input_double_register_46", DOUBLE},
	{"input_double_register_47", DOUBLE},

	{ "tool_output_mode",		UINT8}, 	/* The current output mode 	5.2.0 */
	{ "tool_digital_output0_mode", 	UINT8}, 	/* The current mode of digital output 0 	5.2.0 */
	{ "tool_digital_output1_mode", 	UINT8}, 	/* The current mode of digital output 1 	5.2.0 */
	{ "input_bit_registers0_to_31", UINT32}, 	/* General purpose bits (input read back) 	3.4.0 */
	{ "input_bit_registers32_to_63",UINT32}, 	/* General purpose bits (input read back) 	3.4.0 */
};

static inline int output_name_to_idx(const char *name) {
	for (size_t i = 0; i < sizeof(rtde_output_names)/sizeof(struct rtde_data_names); i++) {
		if (strcmp(name, rtde_output_names[i].name) == 0)
			return i;
	}
	return -1;
}

static inline enum RTDE_DATA_TYPE output_name_to_type(const char *name) {
	int idx = output_name_to_idx(name);
	if (idx < 0)
		return NOT_FOUND;
	return rtde_output_names[idx].type;
}
#ifdef  __cplusplus
}
#endif
#endif	/* MAGIC_H */
