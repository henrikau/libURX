/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#ifndef HEADER_H
#define HEADER_H

// FIXME:
// This module is *ripe* for some C++ magic!

#include <urx/magic.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <stdio.h>

struct rtde_header {
	uint16_t size;
	uint8_t type;
} __attribute__((packed));

struct rtde_prot {
	struct rtde_header hdr;
	union {
		uint16_t version;
		uint8_t accepted;
	} payload;
} __attribute__((packed));


struct rtde_ur_ver_payload {
	uint32_t major;
	uint32_t minor;
	uint32_t bugfix;
	uint32_t build;
} __attribute__((packed));

struct rtde_ur_ver {
	struct rtde_header hdr;
	struct rtde_ur_ver_payload payload;
} __attribute__((packed));

struct rtde_msg {
	struct rtde_header hdr;
	uint8_t mlength;
	/* this is an array, but placed directly at this address, this
	 * conveniently counts for \0 when allocating */
	unsigned char msg;
} __attribute__((packed));


/*-------------------------------------------------------
 * RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS (.v2)
 *
 * Summary: Setup the outputs recipe. At the moment the CON only
 * supports one output recipe and the output frequency is
 * configurable.
 *
 * The frequency must be between 1 and 125 Hz and the output rate will
 * be according to floor(125 / frequency). The package should contain
 * all desired output variables.
 *
 * The variable names is a list of comma separated variable name
 * strings.  Direction: EE -> CON
 */
struct rtde_control_package_out {
	struct rtde_header hdr;
	double update_freq;
	unsigned char variables;
} __attribute__((packed));

/*-------------------------------------------------------
 * RTDE_CONTROL_PACKAGE_SETUP_INPUTS (.v1 && .v2)
 *
 * Summary: Setup the input recipe. CON supports up to 255 different
 * input recipies (valid recipe_id: 1..255)
 *
 * The variable names is a list of comma separated variable name
 * strings.  Direction: CON -> EE
 */
struct rtde_control_package_in {
	struct rtde_header hdr;
	unsigned char variables;
} __attribute__((packed));

/*
 * Summary: Returns the variable types in the same order as they were
 * supplied in the request.
 *
 * Variable types:
 *
 * VECTOR6D, VECTOR3D, VECTOR6INT32, VECTOR6UINT32, DOUBLE, UINT64,
 * UINT32, INT32, BOOL, UINT8
 *
 * If a variable is not available, then the variable type will be
 * "NOT_FOUND".  In case of one or more "NOT_FOUND" return values, the
 * recipe is considered invalid and the RTDE will not output this data.
 */
struct rtde_control_package_resp {
	struct rtde_header hdr;
	uint8_t recipe_id;
	unsigned char variables;
} __attribute__((packed));


struct rtde_data_package {
	struct rtde_header hdr;
	uint8_t recipe_id;
	unsigned char data;
} __attribute__((packed));

// control pacakge, start/pause response
struct rtde_control_package_sp_resp {
	struct rtde_header hdr;
	bool accepted;
} __attribute__((packed));


/**
 * \brief Acquire a reference to a RTDE data package
 */
struct rtde_header * rtde_hdr_get(enum RTDE_PACKAGE_TYPE ptype);
void rtde_hdr_put(struct rtde_header * rtde_hdr_get);

static inline bool basic_hdr_validate(struct rtde_header *hdr, uint8_t type)
{
	if (!hdr)
		return false;

	uint16_t sz = ntohs(hdr->size);
	if (sz < sizeof(*hdr) || sz > 2000)
		return false;
	return hdr->type == type;
}

bool rtde_mkreq_prot_version(struct rtde_prot *);
bool rtde_parse_prot_version(struct rtde_prot *);
bool rtde_mkreq_urctrl_version(struct rtde_header *);
bool rtde_parse_urctrl_resp(struct rtde_ur_ver *, struct rtde_ur_ver_payload *);


/* msgsz and srcsz must include space for \0 */
bool rtde_mkreq_msg_out(struct rtde_msg *rtde,
			const char *msg, size_t msgsz,
			const char *src, size_t srcsz);

struct rtde_msg * rtde_msg_get(void);
static inline void rtde_msg_put(struct rtde_msg *msg) { rtde_hdr_put((struct rtde_header *)msg); }

void rtde_control_msg_clear(struct rtde_header *hdr, bool dir_out);

struct rtde_control_package_out * rtde_control_msg_get(double freq);

/**
 * \brief: create a CONTROL_PACKAGE_INPUTS message of default size
 *
 */
struct rtde_control_package_in * rtde_control_get_in(void);
static inline void rtde_control_put_in(struct rtde_control_package_in *msg) { rtde_hdr_put((struct rtde_header *)msg); }


/**
 * \brief Setup output/input in a single step
 *
 * set payload in one go, assumes caller know its business and that the
 * fields are all valid. Will update rest of the headers accordingly
 * \param[in,out] cp		target control packet
 * \param[in]	  payload	the target payload
 * \param[in]	  dir_out	direction (dir_out: output from controller, false -> input)
 */
bool rtde_control_msg_in(struct rtde_control_package_in *in, const char *payload);
bool rtde_control_msg_out(struct rtde_control_package_out *in, const char *payload);

void rtde_control_msg_finalize(struct rtde_control_package_out *);
static inline void rtde_control_msg_put(struct rtde_control_package_out *msg) { rtde_hdr_put((struct rtde_header *)msg); }

/**
 * \brief validate a Control Package response
 *
 * Will do basic sanitization of the header and make sure all fields in
 * the response is valid types. If NOT_FOUND is present, it will
 * invalidate the response as the response_id is invalid and cannot be
 * used.
 *
 * \resp [in] package to validate
 * \return true if package looks good.
 */
bool rtde_control_package_resp_validate(struct rtde_control_package_resp *resp);

static inline struct rtde_data_package * rtde_data_package_get(void)
{
	return (struct rtde_data_package *)rtde_hdr_get(RTDE_DATA_PACKAGE);
}
static inline void rtde_data_package_put(struct rtde_data_package *dp) { rtde_hdr_put((struct rtde_header *)dp); }

void rtde_data_package_init(struct rtde_data_package *dp, int recipe_id, int bytes);

bool rtde_parse_data_package(struct rtde_data_package *data);

void rtde_control_package_start(struct rtde_header *sp);
void rtde_control_package_stop(struct rtde_header *sp);

/* control package start/pause response */
bool rtde_control_package_sp_resp_validate(
	struct rtde_control_package_sp_resp *resp,
	bool start);

#endif /* HEADER_H */
