#define BOOST_TEST_MODULE recpie_test
#define BOOST_TEST_DYN_LINK
#include <urx/rtde_recipe.hpp>
#include <urx/helper.hpp>
#include "test_helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <stdint.h>
struct F
{
    F() :
        r()
    {}

    ~F() {}
    urx::RTDE_Recipe r;
};
BOOST_FIXTURE_TEST_SUITE(s, F)

BOOST_AUTO_TEST_CASE(test_recipe_first)
{
    BOOST_CHECK(!r.active());
    BOOST_CHECK(r.recipe_id() == -1);
}

BOOST_AUTO_TEST_CASE(test_recipe_add)
{
    double ts;
    BOOST_CHECK(r.add_field("timestamp", &ts));
    BOOST_CHECK(!r.add_field("joint_mode", NULL));

    double bamboo;
    BOOST_CHECK(!r.add_field("bamboo", &bamboo));

    int32_t joint_mode[6];
    BOOST_CHECK(r.add_field("joint_mode", joint_mode));

    BOOST_CHECK(r.num_fields() == 2);
    BOOST_CHECK(r.get_fields() == "timestamp,joint_mode");

    int32_t robot_mode;
    BOOST_CHECK(r.add_field("robot_mode", &robot_mode));
    BOOST_CHECK(r.num_fields() == 3);
    BOOST_CHECK(r.get_fields() == "timestamp,joint_mode,robot_mode");
}

BOOST_AUTO_TEST_CASE(test_recipe_clear_fields)
{
    int32_t joint_mode[6];
    int32_t robot_mode;
    BOOST_CHECK(r.add_field("joint_mode", joint_mode));
    BOOST_CHECK(r.add_field("robot_mode", &robot_mode));
    BOOST_CHECK(r.num_fields() == 2);
    r.clear_fields();
    BOOST_CHECK(r.num_fields() == 0);
}

BOOST_AUTO_TEST_CASE(test_recipe_construct_msg)
{
    double ts;
    BOOST_CHECK(r.add_field("timestamp", &ts));
    BOOST_CHECK(r.num_fields() == 1);
    struct rtde_control_package_out *cpo = (struct rtde_control_package_out *)r.GetMsg();
    BOOST_CHECK(cpo!= NULL);
    BOOST_CHECK(strcmp((const char *)rtde_control_package_out_get_payload(cpo), "timestamp") == 0);
}

BOOST_AUTO_TEST_CASE(test_recipe_response)
{
    double ts;
    BOOST_CHECK(r.add_field("timestamp", &ts));

    BOOST_CHECK(!r.register_response(NULL));
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "!DOUBLE", 42);
    BOOST_CHECK(!r.register_response(resp));

    _set_recipe_resp(resp, "DOUBLE", 42);
    BOOST_CHECK(r.register_response(resp));

    double tq[6];
    BOOST_CHECK(r.add_field("target_q", tq));
    BOOST_CHECK(!r.register_response(resp));
    _set_recipe_resp(resp, "DOUBLE,VECTOR6D", 42);
    BOOST_CHECK(r.register_response(resp));

    _set_recipe_resp(resp, "DOUBLE,DOUBLE", 42);
    BOOST_CHECK(!r.register_response(resp));
}

BOOST_AUTO_TEST_CASE(test_recipe_response_etx)
{
    double ts;
    BOOST_CHECK(r.add_field("timestamp", &ts));
    struct rtde_control_package_resp *resp = create_cp_resp();
    _set_recipe_resp(resp, "DOUBLEE", 42);
    unsigned char *payload = rtde_control_package_resp_get_payload(resp);
    payload[6] = 0x03; // End-of-Text
    BOOST_CHECK(r.register_response(resp));

}

BOOST_AUTO_TEST_CASE(test_parse_full_recipe)
{
    BOOST_CHECK(!r.parse(NULL));
    unsigned char buffer[128];
    memset(buffer, 0, 128);
    printf("----------------------------------------------\n");
    double ts;
    double *ts_ptr = (double *)&buffer[0];
    BOOST_CHECK(r.add_field("timestamp", &ts));

    int32_t jm[6];
    int32_t *jm_ptr = (int32_t *)&buffer[8];
    BOOST_CHECK(r.add_field("joint_mode", jm));

    int32_t rm;
    int32_t rm_val;
    int32_t *rm_ptr = (int32_t *)&buffer[8 + 6*4];
    BOOST_CHECK(r.add_field("robot_mode", &rm));

    double jt[6];
    double jt_val[6];
    double *jt_ptr = (double *)&buffer[8 + 6*4 + 4];
    BOOST_CHECK(r.add_field("joint_temperatures", jt));

    *ts_ptr = urx::double_h(42.1337);
    for (int c = 0; c < 6; ++c)
        jm_ptr[c] = htobe32(c+2);

    rm_val = 1;
    rm_ptr[0] = htobe32(rm_val);

    for (int c = 0; c < 6; ++c) {
        jt_val[c] = (c+123.456)*0.983;
        jt_ptr[c] =  urx::double_h(jt_val[c]);
    }

    BOOST_CHECK(r.expected_bytes() == (8 + 6*4 + 4 + 6*8));
    BOOST_CHECK(r.parse(buffer));
    BOOST_CHECK_CLOSE(ts, 42.1337, 0.00001);

    for (int c = 0; c < 6; ++c) {
        //printf("jm[%d]=%d\n", c, jm[c]);
        BOOST_CHECK(jm[c] == c+2 );
    }

    BOOST_CHECK(rm == rm_val);
    for (int c = 0; c < 6; ++c){
        //printf("jt[%d]=%f, jt_val[%d]=%f\n", c, jt[c], c, jt_val[c]);
        BOOST_CHECK_CLOSE(jt[c], jt_val[c], 0.00001);
    }
}
BOOST_AUTO_TEST_CASE(test_recipe_change_dir_after_add_fail)
{
    double ts;
    r.add_field("timestamp", &ts);
    BOOST_CHECK(!r.dir_input());
    BOOST_CHECK(r.dir_output());
}

BOOST_AUTO_TEST_CASE(test_recipe_store_out_fail)
{
    BOOST_CHECK(r.dir_input());
    double slf = 1337.42;
    BOOST_CHECK(r.add_field("speed_slider_fraction", &slf));

    unsigned char buf[32] = {0};
    struct rtde_data_package * dp = (struct rtde_data_package *)buf;
    rtde_data_package_init(dp, 1, 8);

    BOOST_CHECK(!r.parse(buf));
    BOOST_CHECK(r.store(dp));
}

BOOST_AUTO_TEST_CASE(test_recipe_parse_in_fail)
{
    double ts;
    r.add_field("timestamp", &ts);
    unsigned char buffer[32] = {0};
    struct rtde_data_package * dp = (struct rtde_data_package *)buffer;

    BOOST_CHECK(!r.store(dp));
}

BOOST_AUTO_TEST_CASE(test_recipe_in_construct_msg)
{
    r.dir_input();
    double slf = 1337.42;
    r.add_field("speed_slider_fraction", &slf);
    uint32_t slm = 0xdeadbeef;
    BOOST_CHECK(r.add_field("speed_slider_mask", &slm));
    BOOST_CHECK(strcmp(r.get_fields().c_str(), "speed_slider_fraction,speed_slider_mask") == 0);

    struct rtde_control_package_in *cpi = (struct rtde_control_package_in *)r.GetMsg();
    BOOST_CHECK(cpi != NULL);
    BOOST_CHECK(strcmp((char *)rtde_control_package_in_get_payload(cpi), "speed_slider_fraction,speed_slider_mask") == 0);
}

BOOST_AUTO_TEST_CASE(test_recipe_in)
{
    BOOST_CHECK(r.dir_input());

    double slf = 1337.42;
    BOOST_CHECK(r.add_field("speed_slider_fraction", &slf));
    BOOST_CHECK(r.num_fields() == 1);

    uint32_t slm = 0xdeadbeef;
    BOOST_CHECK(r.add_field("speed_slider_mask", &slm));
    BOOST_CHECK(r.num_fields() == 2);
    BOOST_CHECK(r.expected_bytes() == 12);

    uint8_t cdo = 0x7a;
    BOOST_CHECK(r.add_field("configurable_digital_output", &cdo));
    BOOST_CHECK(r.num_fields() == 3);
    BOOST_CHECK(r.expected_bytes() == 13);

    unsigned char buf[32] = {0};
    struct rtde_data_package * dp = (struct rtde_data_package *)buf;
    rtde_data_package_init(dp, 1, r.expected_bytes());
    unsigned char *buffer = rtde_data_package_get_payload(dp);

    BOOST_CHECK(!r.store(NULL));
    BOOST_CHECK(r.store(dp));

    BOOST_CHECK_CLOSE(urx::double_h(*(double *)buffer), slf, 0.00001);
    BOOST_CHECK(be32toh(*(uint32_t *)&buffer[8]) == slm);
    BOOST_CHECK(buffer[12] == cdo);
    BOOST_CHECK(buffer[13] == 0x00);
}

// Check that every single byte is correct in a simple INPUT request
BOOST_AUTO_TEST_CASE(test_handler_in_byte_ok)
{
    urx::RTDE_Recipe in = urx::RTDE_Recipe();
    in.dir_input();

    int32_t iir20;
    in.add_field("input_int_register_20", &iir20);
    struct rtde_control_package_in *cp = (struct rtde_control_package_in *)in.GetMsg();
    unsigned char *sbuf = (unsigned char *)cp;

    BOOST_CHECK(sbuf != NULL);
    BOOST_CHECK(in.expected_bytes() == 4);
    BOOST_CHECK(in.get_fields() == "input_int_register_20");

    // Test the buffer, this will catch alignment-issues when writing
    // the buffer and sending it to remote
    BOOST_CHECK_MESSAGE(sbuf[0] == 0x00, "sbuf[0] was " << std::hex << int(sbuf[0]) << " expected 0x00");
    BOOST_CHECK_MESSAGE(sbuf[1] == 0x18, "sbuf[1] was 0x" << std::hex << int(sbuf[1]) << " expected 0x19");
    BOOST_CHECK(sbuf[3] == 0x69);
    BOOST_CHECK(sbuf[4] == 0x6e);
    BOOST_CHECK(sbuf[5] == 0x70);
    BOOST_CHECK(sbuf[6] == 0x75);
    BOOST_CHECK(sbuf[7] == 0x74);
    BOOST_CHECK(sbuf[8] == 0x5f);
    BOOST_CHECK(sbuf[9] == 0x69);
    BOOST_CHECK(sbuf[10] == 0x6e);
    BOOST_CHECK(sbuf[11] == 0x74);
    BOOST_CHECK(sbuf[12] == 0x5f);
    BOOST_CHECK(sbuf[13] == 0x72);
    BOOST_CHECK(sbuf[14] == 0x65);
    BOOST_CHECK(sbuf[15] == 0x67);
    BOOST_CHECK(sbuf[16] == 0x69);
    BOOST_CHECK(sbuf[17] == 0x73);
    BOOST_CHECK(sbuf[18] == 0x74);
    BOOST_CHECK(sbuf[19] == 0x65);
    BOOST_CHECK(sbuf[20] == 0x72);
    BOOST_CHECK(sbuf[21] == 0x5f);
    BOOST_CHECK(sbuf[22] == 0x32);
    BOOST_CHECK(sbuf[23] == 0x30);
}

BOOST_AUTO_TEST_SUITE_END()
