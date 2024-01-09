/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#include <urx/header.hpp>
#include <stdlib.h>
#include <strings.h>		/* bzero */

struct rtde_header * rtde_hdr_get(enum RTDE_PACKAGE_TYPE ptype)
{
	/* all packages to/from the controller can be UP TO 2048 bytes. */
        struct rtde_header * hdr = (struct rtde_header *)calloc(2048, 1);
	if (!hdr)
		return NULL;
	hdr->type = (uint8_t)ptype;
	return hdr;
}

void rtde_hdr_put(struct rtde_header * hdr)
{
	if (hdr) {
		bzero(hdr, 2048); /* compiler will probably rip this out */
		free(hdr);
	}
}

bool rtde_mkreq_prot_version(struct rtde_prot * header)
{
	if (!header)
		return false;

	/* FIXME: Size should not be 0, but until we know what it should be.. */
	header->hdr.size = htons(sizeof(*header));
	header->hdr.type = RTDE_REQUEST_PROTOCOL_VERSION;
	header->payload.version = htons(RTDE_SUPPORTED_PROTOCOL_VERSION);
	return true;
}

bool rtde_parse_prot_version(struct rtde_prot *header)
{
	if (!header ||
		ntohs(header->hdr.size) != 4 ||
		header->hdr.type != RTDE_REQUEST_PROTOCOL_VERSION)
		return false;

	return header->payload.accepted;
}


bool rtde_mkreq_urctrl_version(struct rtde_header *header)
{
	if (!header)
		return false;

	/* construct version request, which is only the header, response
	 * will contain extra bytes will version numberings
	 */
	header->size = htons(sizeof(struct rtde_header));
	header->type = RTDE_GET_URCONTROL_VERSION;

	return true;
}

bool rtde_parse_urctrl_resp(struct rtde_ur_ver *header, struct rtde_ur_ver_payload *payload)
{
	if (!header || !payload)
		return false;

	/* Remote expects a size, avoid error if struct in our end has
	 * been modified (i.e. avoid sizeof(*header))
	 */
	if (ntohs(header->hdr.size) != 19 ||
		header->hdr.type != RTDE_GET_URCONTROL_VERSION)
		return false;

	payload->major  = ntohl(header->payload.major);
	payload->minor  = ntohl(header->payload.minor);
	payload->bugfix = ntohl(header->payload.bugfix);
	payload->build  = ntohl(header->payload.build);

	return true;
}

struct rtde_msg * rtde_msg_get(void)
{
	struct rtde_msg *msg = (struct rtde_msg *)rtde_hdr_get(RTDE_TEXT_MESSAGE);
	if (!msg)
		return NULL;
	return msg;
}

bool rtde_mkreq_msg_out(struct rtde_msg *rtde,
			const char *msg, size_t msgsz,
			const char *src, size_t srcsz)
{
	if (!rtde || !msg || !src || msgsz > 0xff || srcsz > 0xff)
		return false;
	size_t size = sizeof(struct rtde_header) + msgsz + srcsz + sizeof(uint8_t)*3;

	rtde->hdr.type = RTDE_TEXT_MESSAGE;
	rtde->hdr.size = htons(size);
	rtde->mlength = msgsz;

        // rtde->msg is of 0 size, but we know that rtde_msg is mapped
        // to a 2048 byte block, so it is safe to write to these offsets.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
	unsigned char * dst = rtde_msg_get_payload(rtde);
	for (size_t c = 0; c < msgsz; ++c) {
            dst[c] = msg[c];
        }
	dst[msgsz] = srcsz;

	for (size_t c = 0; c < srcsz; ++c)
		dst[msgsz+1+c] = src[c];

	dst[msgsz+1+srcsz] = RTDE_EXCEPTION_MESSAGE;
#pragma GCC diagnostic pop

	return true;
}

struct rtde_control_package_out * rtde_control_msg_get(double freq)
{
	if (freq < 1.0 || freq > 125.0)
		return NULL;

	struct rtde_control_package_out *cp = (struct rtde_control_package_out *)rtde_hdr_get(RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS);
	if (!cp)
		return NULL;

	cp->hdr.size = htons(sizeof(struct rtde_control_package_out));
	cp->update_freq = freq;
	return cp;
}

struct rtde_control_package_in * rtde_control_get_in(void)
{
	struct rtde_control_package_in *in = (struct rtde_control_package_in *)rtde_hdr_get(RTDE_CONTROL_PACKAGE_SETUP_INPUTS);
	if (!in) {
            return NULL;
        }

	in->hdr.size = htons(sizeof(*in));
	return in;
}

void rtde_control_msg_clear(struct rtde_header *hdr, bool dir_out)
{
	if (dir_out) {
		struct rtde_control_package_out *cp = (struct rtde_control_package_out *)hdr;
		hdr->type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
		hdr->size = htons(sizeof(*cp));
		cp->update_freq = 0.0;
		*(rtde_control_package_out_get_payload(cp)) = 0;
	} else {
		struct rtde_control_package_in *cp = (struct rtde_control_package_in *)hdr;
		hdr->size = htons(sizeof(*cp));
		hdr->type = RTDE_CONTROL_PACKAGE_SETUP_INPUTS;
		*(rtde_control_package_in_get_payload(cp)) = 0;
	}
}
bool rtde_control_msg_in(struct rtde_control_package_in *cpi, const char *payload)
{
	if (!cpi || !payload)
		return false;

	size_t len = strlen(payload);
	if (len > (2048 - sizeof(*cpi)))
		return false;

	rtde_control_msg_clear((struct rtde_header *)cpi, false);

        unsigned char *variables = rtde_control_package_in_get_payload(cpi);
	for (size_t i = 0; i < len; i++)
		variables[i] = payload[i];
	variables[len] = 0x00;
	cpi->hdr.size = htons(ntohs(cpi->hdr.size) + len);
	return true;
}

bool rtde_control_msg_out(struct rtde_control_package_out *cpo, const char *payload)
{
	if (!cpo || !payload)
		return false;

	size_t len = strlen(payload);
	if (len > (2048 - sizeof(*cpo)))
		return false;

	rtde_control_msg_clear((struct rtde_header *)cpo, true);
        unsigned char *variables = rtde_control_package_out_get_payload(cpo);
	for (size_t i = 0; i < len; i++)
		variables[i] = payload[i];
	variables[len] = 0x00;
	cpo->hdr.size = htons(ntohs(cpo->hdr.size) + len);
	return true;
}

void rtde_control_msg_finalize(struct rtde_control_package_out *cpo)
{
	int sz = ntohs(cpo->hdr.size);
	if (sz == sizeof(struct rtde_header))
		return;

	/* drop trailing , */
        (rtde_control_package_out_get_payload(cpo))[sz - sizeof(struct rtde_control_package_out)] = 0;
	cpo->hdr.size = htons(sz);
}

bool rtde_control_package_resp_validate(struct rtde_control_package_resp *resp)
{
	if (!basic_hdr_validate((struct rtde_header *)resp, RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS) &&
            !basic_hdr_validate((struct rtde_header *)resp, RTDE_CONTROL_PACKAGE_SETUP_INPUTS))
		return false;

	/* supersimple token-parser, split on ',' and match on
         * "NOT_FOUND"
         *
         * "At the moment the CON only supports one output recipe."
         * means that the only valid recipe_id should be 1. Emphasis on
         * *should* as it can be 0..255.
         *
         * "In case of one or more "NOT_FOUND" return values, the recipe
         * is considered invalid and the RTDE will not output this
         * data."
         *
         * This, of course, makes parsing a tad difficult as recipe_id
         * can now either be a valid ID in the range <0..255> or 'V',
         * 'D', 'U', 'I', 'B', 'N' (From VECTOR6D, VECTOR3D, VECTOR6INT32,
         * VECTOR6UINT32, DOUBLE, UINT64, UINT32, INT32, BOOL, UINT8,
         * NOT_FOUND)
         *
         * special case, we have a NOT_FOUND, and recipe_id is not used,
         * this is first item, so we will start looking at 'OT_FOUND'.
         * However, data_name_to_type will catch this as OT_FOUND is an
         * unknown and return token NOT_FOUND
	 */
#if 0
        switch (resp->recipe_id) {
        case 'B':
        case 'D':
        case 'I':
        case 'N':
        case 'U':
        case 'V':
            printf("%s: Possibly invalid header, suspucious recipe_id (%d) received\n", __func__, (int)resp->recipe_id);
            break;
        default:
            break;
        }
#endif
	unsigned char *start = rtde_control_package_resp_get_payload(resp);
	unsigned char *ptr = start;
	unsigned char *end = start + htons(resp->hdr.size) - sizeof(struct rtde_control_package_resp);
	while (ptr < end) {
		if (*ptr == ',' || *(ptr+1) == 0x00) {
			enum RTDE_DATA_TYPE type = data_name_to_type((const char *)start, (size_t)(ptr-start));
			if (type == NOT_FOUND || type == IN_USE)
                            return false;

			start = ++ptr;
		}
		++ptr;
	}
	return true;
}

void rtde_data_package_init(struct rtde_data_package *dp, int recipe_id, int bytes)
{
    if (!dp)
        return;
    dp->hdr.type = RTDE_DATA_PACKAGE;
    dp->hdr.size = htons(sizeof(*dp) + bytes);
    dp->recipe_id = recipe_id;
}

bool rtde_parse_data_package(struct rtde_data_package *data)
{
	if (!data)
		return false;
	else if (data->recipe_id == 0)
		return false;
	else if (data->hdr.type != RTDE_DATA_PACKAGE)
		return false;

	return ntohs(data->hdr.size) >= sizeof(struct rtde_data_package);
}


static void rtde_control_package_(struct rtde_header *sp, bool start)
{
	if (!sp)
		return ;
	sp->size = htons(sizeof(struct rtde_header));
	if (start)
		sp->type = RTDE_CONTROL_PACKAGE_START;
	else
		sp->type = RTDE_CONTROL_PACKAGE_PAUSE;
}

void rtde_control_package_start(struct rtde_header *sp)
{
    rtde_control_package_(sp, true);
}
void rtde_control_package_stop(struct rtde_header *sp)
{
    rtde_control_package_(sp, false);
}

bool rtde_control_package_sp_resp_validate(

	struct rtde_control_package_sp_resp *resp,
	bool start)
{
	if (!resp)
		return false;
	else if (ntohs(resp->hdr.size) != 4)
		return false;
	else if (start && resp->hdr.type != RTDE_CONTROL_PACKAGE_START)
		return false;
	else if (!start && resp->hdr.type != RTDE_CONTROL_PACKAGE_PAUSE)
		return false;
	return true;
}
