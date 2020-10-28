/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE header_test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <urx/magic.h>
#include <arpa/inet.h>

#include <urx/header.hpp>
#include "test_helpers.hpp"

BOOST_AUTO_TEST_SUITE(header_test)

BOOST_AUTO_TEST_CASE(test_header_verify_struct_sizes)
{
    BOOST_CHECK(sizeof(struct rtde_header) == 3);
    BOOST_CHECK(sizeof(struct rtde_prot) == 5);
    BOOST_CHECK(sizeof(struct rtde_ur_ver_payload) == 16);
    BOOST_CHECK(sizeof(struct rtde_ur_ver) == 19);
    BOOST_CHECK(sizeof(struct rtde_msg) == 5);
    BOOST_CHECK(sizeof(struct rtde_control_package_out) == 12);
    BOOST_CHECK(sizeof(struct rtde_control_package_in) == 4);
    BOOST_CHECK(sizeof(struct rtde_control_package_resp) == 5);
    BOOST_CHECK(sizeof(struct rtde_data_package) == 5);
    BOOST_CHECK(sizeof(struct rtde_control_package_sp_resp) == 4);
}

BOOST_AUTO_TEST_CASE(test_header_setup)
{
  BOOST_CHECK(RTDE_REQUEST_PROTOCOL_VERSION=='V');
  BOOST_CHECK(RTDE_GET_URCONTROL_VERSION=='v');
  BOOST_CHECK(RTDE_TEXT_MESSAGE=='M');
  BOOST_CHECK(RTDE_DATA_PACKAGE=='U');
  BOOST_CHECK(RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS=='O');
  BOOST_CHECK(RTDE_CONTROL_PACKAGE_SETUP_INPUTS=='I');
  BOOST_CHECK(RTDE_CONTROL_PACKAGE_START=='S');
  BOOST_CHECK(RTDE_CONTROL_PACKAGE_PAUSE=='P');

  BOOST_CHECK(sizeof(struct rtde_header) == 3);
}

BOOST_AUTO_TEST_CASE(test_header_create)
{
  struct rtde_prot header;
  BOOST_CHECK(rtde_mkreq_prot_version(&header));
  BOOST_CHECK(header.hdr.type == RTDE_REQUEST_PROTOCOL_VERSION);

  // We are on LE, headers encoded for B.E.
  BOOST_CHECK(header.hdr.size  == 0x0500);
  BOOST_CHECK(header.payload.version == 0x0200);
}

void _valid_prot_hdr(struct rtde_prot *header)
{
  header->hdr.size = htons(4);
  header->payload.accepted = true;
  header->hdr.type = RTDE_REQUEST_PROTOCOL_VERSION;
}

BOOST_AUTO_TEST_CASE(test_header_resp)
{
  BOOST_CHECK(!rtde_parse_prot_version(NULL));

  struct rtde_prot header;

  _valid_prot_hdr(&header);
  BOOST_CHECK(rtde_parse_prot_version(&header));

  // various invalid frames
  _valid_prot_hdr(&header);
  header.hdr.size = 0;
  BOOST_CHECK(!rtde_parse_prot_version(&header));

  _valid_prot_hdr(&header);
  header.hdr.type = RTDE_GET_URCONTROL_VERSION;
  BOOST_CHECK(!rtde_parse_prot_version(&header));

  _valid_prot_hdr(&header);
  header.payload.accepted = false;
  BOOST_CHECK(!rtde_parse_prot_version(&header));
}

BOOST_AUTO_TEST_CASE(test_header_get_ur_ver)
{
  BOOST_CHECK(!rtde_mkreq_urctrl_version(NULL));

  struct rtde_header urver;
  BOOST_CHECK(rtde_mkreq_urctrl_version(&urver));
  BOOST_CHECK(urver.type == RTDE_GET_URCONTROL_VERSION);
  BOOST_CHECK(urver.size  == 0x0300);
}

void _valid_ur_hdr(struct rtde_ur_ver *header)
{
  printf("headersize: %zu\n", sizeof(*header));
  memset(header, 0, sizeof(*header));
  header->hdr.size = htons(sizeof(*header));
  header->hdr.type = RTDE_GET_URCONTROL_VERSION;

  header->payload.major = htonl(5);
  header->payload.minor = htonl(3);
  header->payload.bugfix = htonl(42);
  header->payload.build = htonl(1337);
}

BOOST_AUTO_TEST_CASE(test_header_ur_ver_resp)
{
  struct rtde_ur_ver header;
  struct rtde_ur_ver_payload payload;

  BOOST_CHECK(!rtde_parse_urctrl_resp(NULL, NULL));
  BOOST_CHECK(!rtde_parse_urctrl_resp(&header, NULL));
  BOOST_CHECK(!rtde_parse_urctrl_resp(NULL, &payload));

  _valid_ur_hdr(&header);
  header.hdr.size = 0;
  BOOST_CHECK(!rtde_parse_urctrl_resp(&header, &payload));

  _valid_ur_hdr(&header);
  header.hdr.type = RTDE_REQUEST_PROTOCOL_VERSION;
  BOOST_CHECK(!rtde_parse_urctrl_resp(&header, &payload));

  _valid_ur_hdr(&header);
  BOOST_CHECK(rtde_parse_urctrl_resp(&header, &payload));
  BOOST_CHECK(payload.major == 5);
  BOOST_CHECK(payload.minor == 3);
  BOOST_CHECK(payload.bugfix == 42);
  BOOST_CHECK(payload.build == 1337);
}

BOOST_AUTO_TEST_CASE(test_header_msg)
{
    const char *msg = "hello, world";
    const char *src = "the Universe and everything";
    char longmsg[256] = {0};
    memset(longmsg, 'a', 255);

    struct rtde_msg *rtde = rtde_msg_get();
    BOOST_CHECK(rtde != NULL);
    BOOST_CHECK(!rtde_mkreq_msg_out(rtde, msg, -1, src, strlen(src)+1));
    BOOST_CHECK(!rtde_mkreq_msg_out(rtde, longmsg, 256, src, strlen(src)+1));
    BOOST_CHECK( rtde_mkreq_msg_out(rtde, msg, 5, src, 3));
    BOOST_CHECK( rtde_mkreq_msg_out(rtde, longmsg, 255, src, strlen(src)));

    BOOST_CHECK(!rtde_mkreq_msg_out(NULL, msg,  strlen(msg)+1, src, strlen(src)+1));
    BOOST_CHECK(!rtde_mkreq_msg_out(rtde, NULL, strlen(msg)+1, src, strlen(src)+1));
    BOOST_CHECK(!rtde_mkreq_msg_out(rtde, msg,  strlen(msg)+1, NULL , strlen(src)+1));

    // --------------------------------------------------------------
    // make sure header has correct attributes
    //
    // pain that strlen does not include \0
    BOOST_CHECK(rtde_mkreq_msg_out(rtde, msg, strlen(msg)+1, src, strlen(src)+1));
    BOOST_CHECK(rtde->hdr.type == RTDE_TEXT_MESSAGE);

    BOOST_CHECK(ntohs(rtde->hdr.size) == (sizeof(struct rtde_header) + sizeof(uint8_t)*5 + strlen(msg) + strlen(src)));

    BOOST_CHECK(rtde->mlength == (strlen(msg)+1));

    // msg copied properly?
    BOOST_CHECK(strncmp((const char *)&rtde->msg, msg, strlen(msg)) == 0);
    rtde_msg_put(rtde);
}
BOOST_AUTO_TEST_CASE(test_header_types)
{
    BOOST_CHECK(type_to_size(BOOL) == 1);
    BOOST_CHECK(type_to_size(DOUBLE) == 8);
    BOOST_CHECK(type_to_size(UINT8) == 8/8);
    BOOST_CHECK(type_to_size(UINT32) == 32/8);
    BOOST_CHECK(type_to_size(UINT64) == 64/8);
    BOOST_CHECK(type_to_size(INT32) == 32/8);
    BOOST_CHECK(type_to_size(DOUBLE) == 64/8);
    BOOST_CHECK(type_to_size(VECTOR3D) == 3*64/8);
    BOOST_CHECK(type_to_size(VECTOR6D) == 6*64/8);
    BOOST_CHECK(type_to_size(VECTOR6INT32) == 6*32/8);
    BOOST_CHECK(type_to_size(VECTOR6UINT32) == 6*32/8);
    BOOST_CHECK(type_to_size(STRING) == 1);
}

BOOST_AUTO_TEST_CASE(test_data_name)
{
    BOOST_CHECK(output_name_to_type("timestamp") == DOUBLE);
    BOOST_CHECK(type_to_size(output_name_to_type("meh")) == 0);
    BOOST_CHECK(type_to_size(output_name_to_type("actual_robot_voltage")) == 8);
    BOOST_CHECK(type_to_size(output_name_to_type("Actual_robot_voltage")) == 0);
}

BOOST_AUTO_TEST_CASE(test_data_package_freq)
{
    BOOST_CHECK(rtde_control_msg_get(1000.0) == NULL);
    BOOST_CHECK(rtde_control_msg_get(0.0) == NULL);
    BOOST_CHECK(rtde_control_msg_get(125.1) == NULL);
    BOOST_CHECK(rtde_control_msg_get(-40) == NULL);

    struct rtde_control_package_out *cp = rtde_control_msg_get(125.0);
    BOOST_CHECK(cp != NULL);
    BOOST_CHECK(cp->update_freq == 125.0);
    rtde_control_msg_put(cp);
}

BOOST_AUTO_TEST_CASE(test_data_package_fast)
{
    BOOST_CHECK(!rtde_control_msg_out(NULL, "DOUBLE"));

    struct rtde_control_package_out *cp = rtde_control_msg_get(125);
    BOOST_CHECK(cp != NULL);
    BOOST_CHECK(rtde_control_msg_out(cp, "DOUBLE"));

    std::string msg = "DOUBLE,INT32,VECTOR6D,VECTOR6UINT32,VECTOR6UINT32";
    BOOST_CHECK(rtde_control_msg_out(cp, msg.c_str()));
    BOOST_CHECK(ntohs(cp->hdr.size) ==  (sizeof(struct rtde_control_package_out) + msg.length() - 1));
    BOOST_CHECK(!(strcmp((char *)&cp->variables, "error") == 0));
    BOOST_CHECK(strcmp((char *)&cp->variables, msg.c_str()) == 0);
    free(cp);
}

BOOST_AUTO_TEST_CASE(test_basic_data_package_resp)
{
    struct rtde_control_package_resp resp;
    resp.hdr.type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
    resp.hdr.size = htons(sizeof(struct rtde_control_package_resp) - sizeof(unsigned char));
    resp.recipe_id = 1;
    BOOST_CHECK(rtde_control_package_resp_validate(&resp));
    BOOST_CHECK(!rtde_control_package_resp_validate(NULL));

    resp.hdr.type = RTDE_TEXT_MESSAGE;
    BOOST_CHECK(!rtde_control_package_resp_validate(&resp));
    resp.hdr.type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
    resp.hdr.size = 0xffff;
    BOOST_CHECK(!rtde_control_package_resp_validate(&resp));
}

BOOST_AUTO_TEST_CASE(test_data_not_found)
{
    // "In case of one or more "NOT_FOUND" return values, the recipe is
    // considered invalid and the RTDE will not output this data."
    struct rtde_control_package_resp *resp = create_cp_resp();
    char *dst = (char *)resp + sizeof(struct rtde_header);

    BOOST_CHECK(resp != NULL);
    resp->hdr.type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;

    std::string pl = "NOT_FOUND";
    resp->variables = 0;
    strncat(dst, pl.c_str(), pl.length());
    resp->hdr.size = htons(sizeof(*resp) + pl.length()-1);
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    std::string pl2 = "DOUBLE,VECTOR6D,NOT_FOUND,INT32";
    resp->variables = 0;
    strncat(dst, pl2.c_str(), pl2.length());
    resp->hdr.size = htons(sizeof(*resp) + pl2.length()-1);
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    // This is actually an invalid response with not_found, but keep it
    // to avoid regressing (recipe_id should be dropped when NOT_FOUND
    // is present)
    std::string pl3 = "DOUBLE,VECTOR6D,NOT_FOUND,INT32";
    dst = (char *)resp + sizeof(struct rtde_header) + sizeof(uint8_t);
    resp->variables = 0;
    resp->recipe_id = 1;
    strncat(dst, pl3.c_str(), pl3.length());
    resp->hdr.size = htons(sizeof(*resp) + pl3.length());
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));
    free(resp);
}

BOOST_AUTO_TEST_CASE(test_data_package_resp_variables)
{
    struct rtde_control_package_resp *resp = create_cp_resp();

    // Size of usngied char in struct will accomodate for \0
    _set_recipe_resp(resp, "DOUBLE", 1);
    BOOST_CHECK(rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "NOT_FOUND", 1);
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DOUBLE,NOT_FOUND", 1);
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DDOUBLE", 1); // typo
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DOUBLE,INT32,VECTOR6D,", 1);
    BOOST_CHECK(rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DOUBLE,INT32,VECTOR6D,VECTOR6UINT32,VECTOR6UINT32", 1);
    BOOST_CHECK(rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DOUBLE,INT32,VECTOR6D,VECTOR6UINT32,NOT_FOUND,VECTOR6UINT32", 1);
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));

    _set_recipe_resp(resp, "DOUBLE,INT32,VECTOR6D,VECTOR6UlNT32,,VECTOR6UINT32", 1); // typo '..lNT32'
    BOOST_CHECK(!rtde_control_package_resp_validate(resp));
    free(resp);
}

BOOST_AUTO_TEST_CASE(test_data_package_start)
{
    struct rtde_header cp;;
    rtde_control_package_start(&cp);
    BOOST_CHECK(cp.type == RTDE_CONTROL_PACKAGE_START);
    BOOST_CHECK(ntohs(cp.size) == 3);

    rtde_control_package_stop(&cp);
    BOOST_CHECK(cp.type == RTDE_CONTROL_PACKAGE_PAUSE);
}

BOOST_AUTO_TEST_CASE(test_data_package_start_resp)
{
    struct rtde_control_package_sp_resp cpr;
    rtde_control_package_start((struct rtde_header *)&cpr);

    BOOST_CHECK(!rtde_control_package_sp_resp_validate(NULL, true));
    BOOST_CHECK(!rtde_control_package_sp_resp_validate(NULL, false));

    // wrong size
    BOOST_CHECK(!rtde_control_package_sp_resp_validate(&cpr, true));

    cpr.hdr.size = htons(4);
    BOOST_CHECK(rtde_control_package_sp_resp_validate(&cpr, true));

    // wrong type
    cpr.hdr.type = RTDE_TEXT_MESSAGE;
    BOOST_CHECK(!rtde_control_package_sp_resp_validate(&cpr, true));

    // check pause response
    rtde_control_package_stop((struct rtde_header *)&cpr);
    cpr.hdr.size = htons(4);
    cpr.accepted = true;
    BOOST_CHECK(!rtde_control_package_sp_resp_validate(&cpr, true));
    BOOST_CHECK(rtde_control_package_sp_resp_validate(&cpr, false));
}

BOOST_AUTO_TEST_CASE(test_msg_clear)
{
    struct rtde_control_package_out out;
    struct rtde_control_package_in in;
    rtde_control_msg_clear((struct rtde_header *)&out, true);
    rtde_control_msg_clear((struct rtde_header *)&in, false);
    BOOST_CHECK(out.hdr.type == RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS);
    BOOST_CHECK(in.hdr.type == RTDE_CONTROL_PACKAGE_SETUP_INPUTS);
}

BOOST_AUTO_TEST_CASE(test_msg_in_get)
{
    struct rtde_control_package_in *in = rtde_control_get_in();
    BOOST_CHECK(in != NULL);
    BOOST_CHECK(in->hdr.type == RTDE_CONTROL_PACKAGE_SETUP_INPUTS);
    BOOST_CHECK(ntohs(in->hdr.size) == 3);
    rtde_control_put_in(in);
}

BOOST_AUTO_TEST_CASE(test_data_input_names)
{
    BOOST_CHECK(input_name_to_idx("not_present") == -1);
    BOOST_CHECK(input_name_to_idx("speed_slider_mask") == 0);

}

BOOST_AUTO_TEST_CASE(test_data_input_types)
{
    BOOST_CHECK(input_name_to_type("speed_slider_mask") == UINT32);
    BOOST_CHECK(rtde_datasize[input_name_to_type("speed_slider_mask")] == 4);
    BOOST_CHECK(rtde_datasize[input_name_to_type("input_int_register_24")] == 4);
    BOOST_CHECK(rtde_datasize[input_name_to_type("input_double_register_47")] == 8);
}

BOOST_AUTO_TEST_SUITE_END()
