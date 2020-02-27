/*
 * Copyright 2020 SINTEF AS
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/
 */
#define BOOST_TEST_MODULE recpie_token_test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <urx/helper.hpp>
#include <urx/rtde_recipe_token.hpp>

BOOST_AUTO_TEST_SUITE(recipe_token_test)

BOOST_AUTO_TEST_CASE(test_recipe_token_parsing_compound)
{
    int32_t robot_mode;
    urx::RTDE_Recipe_Token_Out rt("robot_mode", 18, &robot_mode);
    BOOST_CHECK(rt.datasize == 4);
    rt.set_offset(9);
    BOOST_CHECK(rt.offset == 9);

    unsigned char buffer[128];
    memset(buffer, 0, 128);
    *(double *)&buffer[0] = 42.1337;
    *(uint8_t *)&buffer[8] = 3;
    *(int32_t *)&buffer[9] = htobe32(0xdeadbeef);
    *(uint32_t *)&buffer[13] = -1;

    BOOST_CHECK(rt.parse(buffer));
    BOOST_CHECK(robot_mode == (int32_t)0xdeadbeef);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_parsing_generic)
{
    double ts;

    urx::RTDE_Recipe_Token_Out rt("timestamp", 0, (void *)&ts);
    double val = 42.1337;
    double data = urx::double_h(val);
    BOOST_CHECK(rt.parse((unsigned char *)&data));
    BOOST_CHECK_CLOSE(ts, val, 0.00001);

    val = 123;
    data = urx::double_h(val);
    BOOST_CHECK(rt.parse((unsigned char *)&data));
    BOOST_CHECK_CLOSE(ts, val, 0.00001);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_parser_generic_bool)
{
    bool t;
    urx::RTDE_Recipe_Token_Out rt("output_bit_register_X",
                                   output_name_to_idx("output_bit_register_X"),
                                   (void *)&t);
    unsigned char buffer[8];
    memset(buffer, 0, 8);
    buffer[3] = 1;
    rt.parse(buffer);
    BOOST_CHECK(!t);

    rt.set_offset(3);
    rt.parse(buffer);
    BOOST_CHECK(t);

    buffer[7] = 0xff;
    rt.set_offset(7);
    BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_parser_multiple)
{
   unsigned char buffer[32];
   memset(buffer, 0, 32);

   bool obrx;
   bool *obrx_ptr = (bool *)&buffer[0];
   urx::RTDE_Recipe_Token_Out obrx_rt("output_bit_register_X",
                                       output_name_to_idx("output_bit_register_X"),
                                       (void *)&obrx);
   uint8_t tom;
   uint8_t *tom_ptr = (uint8_t *)&buffer[1];
   urx::RTDE_Recipe_Token_Out tom_rt("tool_output_mode",
                                      output_name_to_idx("tool_output_mode"),
                                      (void *)&tom);
   tom_rt.set_offset(1);

   int32_t tov;
   int32_t *tov_ptr = (int32_t *)&buffer[2];
   urx::RTDE_Recipe_Token_Out tov_rt("tool_output_voltage",
                                      output_name_to_idx("tool_output_voltage"),
                                      (void *)&tov);
   tov_rt.set_offset(2);

   uint64_t ado;
   uint64_t *ado_ptr = (uint64_t *)&buffer[6];
   urx::RTDE_Recipe_Token_Out ado_rt("actual_digital_output_bits",
                                      output_name_to_idx("actual_digital_output_bits"),
                                      (void *)&ado);
   ado_rt.set_offset(6);

   uint32_t eib;
   uint32_t *eib_ptr = (uint32_t *)&buffer[14];
   urx::RTDE_Recipe_Token_Out eib_rt("euromap67_input_bits",
                                      output_name_to_idx("euromap67_input_bits"),
                                      (void *)&eib);
   eib_rt.set_offset(14);

   double ts;
   double *ts_ptr = (double *)&buffer[18];
   urx::RTDE_Recipe_Token_Out ts_rt("timestamp",
                                     output_name_to_idx("timestamp"),
                                     (void *)&ts);
   ts_rt.set_offset(18);

   obrx_rt.parse(buffer);
   BOOST_CHECK(!obrx);
   *obrx_ptr = 0xff;
   obrx_rt.parse(buffer);
   BOOST_CHECK(obrx);

   tom_rt.parse(buffer);
   BOOST_CHECK(tom == 0);
   *tom_ptr = 0xa0;
   tom_rt.parse(buffer);
   BOOST_CHECK(tom == 0xa0);


   // set data first, parse second to detect overlapping
   double val = 42.0;
   *ts_ptr = urx::double_h(val);
   *tov_ptr = 0x12345678;
   *ado_ptr = htobe64(0xdeadeef00aa00aa);
   *eib_ptr = htobe32(1337);

   tov_rt.parse(buffer);
   ado_rt.parse(buffer);
   eib_rt.parse(buffer);
   ts_rt.parse(buffer);

   BOOST_CHECK(tov = 0x12345678);
   BOOST_CHECK(ado == 0xdeadeef00aa00aa);
   BOOST_CHECK(eib == 1337);
   BOOST_CHECK_CLOSE(ts, 42.0, 0.001);

   *eib_ptr = htobe32(0);
   eib_rt.parse(buffer);
   BOOST_CHECK(eib == 0);

   *eib_ptr = htobe32(1);
   eib_rt.parse(buffer);
   BOOST_CHECK(eib == 1);

}

BOOST_AUTO_TEST_CASE(test_recipe_token_parser_vector3d)
{
   unsigned char buffer[32];
   memset(buffer, 0, 32);

   double jm[3];
   urx::RTDE_Recipe_Token_Out jm_rt("actual_tool_accelerometer",
                                     output_name_to_idx("actual_tool_accelerometer"),
                                     (void *)jm);
   double *jm_ptr = (double *)&buffer[0];
   jm_ptr[0] = urx::double_h(1.0);
   jm_ptr[1] = urx::double_h(219373.1849028482);
   jm_ptr[2] = urx::double_h(-3.0);
   jm_rt.parse(buffer);

   BOOST_CHECK_CLOSE(jm[0], 1.0, 0.0001);
   BOOST_CHECK_CLOSE(jm[1], 219373.1849028482, 0.0001);
   BOOST_CHECK_CLOSE(jm[2], -3.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_parser_vector6d)
{
   unsigned char buffer[50];
   memset(buffer, 0, 50);       // 6 doubles; 48 bytes

   double jt[6];
   urx::RTDE_Recipe_Token_Out jt_rt("joint_temperatures",
                                     output_name_to_idx("joint_temperatures"),
                                     (void *)jt);
   double *jt_ptr = (double *)&buffer[0];
   jt_ptr[0] = urx::double_h(1.0);
   jt_ptr[1] = urx::double_h(2.0);
   jt_ptr[2] = urx::double_h(3.0);
   jt_ptr[3] = urx::double_h(-1.0);
   jt_ptr[4] = urx::double_h(-2.0);
   jt_ptr[5] = urx::double_h(-3.0);
   jt_rt.parse(buffer);
   BOOST_CHECK_CLOSE(jt[0],  1.0, 0.001);
   BOOST_CHECK_CLOSE(jt[1],  2.0, 0.001);
   BOOST_CHECK_CLOSE(jt[2],  3.0, 0.001);
   BOOST_CHECK_CLOSE(jt[3], -1.0, 0.001);
   BOOST_CHECK_CLOSE(jt[4], -2.0, 0.001);
   BOOST_CHECK_CLOSE(jt[5], -3.0, 0.001);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_parser_vector6int32)
{
    int32_t buffer[6];
    int32_t jm[6];

    for (size_t i = 0; i < 6; i++) {
        buffer[i] = htobe32(-i*1337);
        jm[i] = 0;
    }
   urx::RTDE_Recipe_Token_Out jm_rt("joint_mode",
                                     output_name_to_idx("joint_mode"),
                                     (void *)jm);
   jm_rt.parse((unsigned char *)buffer);
   for (int i = 0; i < 6; i++)
       BOOST_CHECK(jm[i] == -i*1337);
}
BOOST_AUTO_TEST_CASE(test_recipe_token_in_store_double)
{
    double slf;
    urx::RTDE_Recipe_Token_In slf_rti("speed_slider_fraction",
                                       input_name_to_idx("speed_slider_fraction"),
                                       (void *)&slf);
    BOOST_CHECK(slf_rti.num_bytes() == 8);
    BOOST_CHECK(slf_rti.offset == 0);

    unsigned char buffer[9] = {0};
    buffer[8] = 0xA0;
    slf = 3.0;
    BOOST_CHECK(slf_rti.store(buffer));
    BOOST_CHECK_CLOSE(slf, 3.0, 0.00001);
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)buffer), slf, 0.000001);

    slf = 1.0;
    BOOST_CHECK(slf_rti.store(buffer));
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)buffer), slf, 0.000001);

    slf = -91.0;
    BOOST_CHECK(slf_rti.store(buffer));
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)buffer), slf, 0.000001);

    slf = 1337.42;
    BOOST_CHECK(slf_rti.store(buffer));
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)buffer), slf, 0.000001);

    // Check canary
    BOOST_CHECK(buffer[8] = 0xA0);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_in_store_uint32)
{
    uint32_t slm;
    urx::RTDE_Recipe_Token_In slm_rti("speed_slider_mask",
                                       input_name_to_idx("speed_slider_mask"),
                                       (void *)&slm);
    BOOST_CHECK(slm_rti.num_bytes() == 4);
    BOOST_CHECK(slm_rti.offset == 0);
    unsigned char buffer[5];
    buffer[4] = 0xff;           // canary
    slm = 0xdeadbeef;
    BOOST_CHECK(slm_rti.store(buffer));
    BOOST_CHECK(slm == 0xdeadbeef);
    BOOST_CHECK(be32toh(*(uint32_t *)buffer) == slm);
    BOOST_CHECK(buffer[4] == 0xff);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_in_store_uint8)
{
    uint8_t cdo;
    urx::RTDE_Recipe_Token_In cdo_rti("configurable_digital_output",
                                       input_name_to_idx("configurable_digital_output"),
                                       (void *)&cdo);
    BOOST_CHECK(cdo_rti.num_bytes() == 1);
    BOOST_CHECK(cdo_rti.offset == 0);
    unsigned char buffer[3];
    // canaries
    buffer[0] = 0xff;
    buffer[2] = 0xa0;
    cdo = 0xfa;
    BOOST_CHECK(cdo_rti.store(&buffer[1]));
    BOOST_CHECK(buffer[0] == 0xff);
    BOOST_CHECK(buffer[1] == 0xfa);
    BOOST_CHECK(buffer[2] == 0xa0);

    cdo_rti.set_offset(1);
    BOOST_CHECK(cdo_rti.offset == 1);
    buffer[0] = 0x00;
    buffer[2] = 0xaa;
    cdo = 0x7f;
    BOOST_CHECK(cdo_rti.store(buffer));
    BOOST_CHECK(buffer[0] == 0x00);
    BOOST_CHECK(buffer[1] == 0x7f);
    BOOST_CHECK(buffer[2] == 0xaa);
}

BOOST_AUTO_TEST_CASE(test_recipe_token_in_store_int32)
{
    int32_t iir3;
    urx::RTDE_Recipe_Token_In iir3_rti("input_int_register_33",
                                       input_name_to_idx("input_int_register_33"),
                                       (void *)&iir3);
    unsigned char buffer[5] = {0};
    buffer[4] = 0xaf;
    iir3 = -1;
    BOOST_CHECK(iir3_rti.store(buffer));
    BOOST_CHECK( (int32_t)be32toh(*(int32_t *)buffer) == -1);
    BOOST_CHECK(buffer[4] == 0xaf);
    memset(buffer, 0, 5);
    buffer[0] = 0x1f;

    iir3 = 0xdeadbeef;
    iir3_rti.set_offset(1);
    BOOST_CHECK(iir3_rti.offset == 1);
    BOOST_CHECK(iir3_rti.store(buffer));
    BOOST_CHECK(buffer[0] == 0x1f);
    BOOST_CHECK(be32toh(*(int32_t *)&buffer[1]) == 0xdeadbeef);
}
BOOST_AUTO_TEST_CASE(test_recipe_token_in_store_NULL)
{
    uint32_t slm;
    urx::RTDE_Recipe_Token_In slm_rti("speed_slider_mask",
                                       input_name_to_idx("speed_slider_mask"),
                                       (void *)NULL);

    BOOST_CHECK(slm_rti.num_bytes() == 0);
    slm_rti = urx::RTDE_Recipe_Token_In("speed_slider_mask",
                                         input_name_to_idx("speed_slider_mask"),
                                         (void *)&slm);
    BOOST_CHECK(slm_rti.num_bytes() == 4);
    BOOST_CHECK(!slm_rti.store(NULL));
}

BOOST_AUTO_TEST_CASE(test_recipe_token_store_multiple)
{
    double slf = 1337.42;
    urx::RTDE_Recipe_Token_In slf_rti("speed_slider_fraction",
                                       input_name_to_idx("speed_slider_fraction"),
                                       (void *)&slf);
    slf_rti.set_offset(0+1);

    uint32_t slm = 0xdeadbeef;
    urx::RTDE_Recipe_Token_In slm_rti("speed_slider_mask",
                                       input_name_to_idx("speed_slider_mask"),
                                       (void *)&slm);
    slm_rti.set_offset(8+1);

    uint8_t cdo = 0x7a;
    urx::RTDE_Recipe_Token_In cdo_rti("configurable_digital_output",
                                       input_name_to_idx("configurable_digital_output"),
                                       (void *)&cdo);
    cdo_rti.set_offset(12+1);

    int32_t iir3 = 0x12345678;
    urx::RTDE_Recipe_Token_In iir3_rti("input_int_register_33",
                                       input_name_to_idx("input_int_register_33"),
                                       (void *)&iir3);
    iir3_rti.set_offset(16+1);

    unsigned char buffer[22] = {0};
    buffer[0]  = 0xa0;
    buffer[21] = 0x0a;

    // Store in random order, offset should work
    BOOST_CHECK(cdo_rti.store(buffer));
    BOOST_CHECK(iir3_rti.store(buffer));
    BOOST_CHECK(slm_rti.store(buffer));
    BOOST_CHECK(slf_rti.store(buffer));

    BOOST_CHECK(buffer[0] = 0xa0);
    BOOST_CHECK_CLOSE(urx::double_be(*(double *)&buffer[1]), slf, 0.000001);
    BOOST_CHECK(be32toh(*(uint32_t *)&buffer[8+1]) == slm);
    BOOST_CHECK(buffer[12+1] == cdo);
    BOOST_CHECK((int32_t)be32toh(*(uint32_t *)&buffer[16+1]) == iir3);
    BOOST_CHECK(buffer[21] = 0x0a);
}
BOOST_AUTO_TEST_SUITE_END()
