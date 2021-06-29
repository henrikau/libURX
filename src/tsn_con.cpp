#include <urx/con.hpp>
void urx::TSNCon::disconnect()
{
    ;
}

bool urx::TSNCon::do_connect(bool)
{
    return true;
}

int urx::TSNCon::do_send(void *buffer, int sz)
{
    talker->add_data((unsigned char *)buffer, sz);
    if (!talker->send())
        return -1;
    return sz;
}

int urx::TSNCon::do_recv(void *rbuf, int sz)
{
    unsigned char *data;
    int rsz = listener->recv(pdu, &data);
    if (rsz < 0)
        return -1;

    // Make sure last byte is 0-terminated
    memcpy(rbuf, data, rsz);
    if (rsz < sz)
        ((unsigned char *)rbuf)[rsz] = 0x00;
    return rsz;
}


int urx::TSNCon::do_send_recv(void *buffer, int, void* rbuf, int rsz)
{
    // This will make you cry...
    // hardcoded recipe, not particularly elegant, but need to
    // trick handler and recipe to set up the parser
    struct rtde_header *hdr = (struct rtde_header *)buffer;

    int recipe_id = 1;
    struct rtde_control_package_resp *resp = (struct rtde_control_package_resp *)rbuf;
    memset(resp, 0, rsz);

    std::string variables;
    if (hdr->type == RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS) {
        variables ="INT32,DOUBLE,VECTOR6D,VECTOR6D,VECTOR6D,VECTOR6D";
        resp->hdr.type = RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
    } else if (hdr->type == RTDE_CONTROL_PACKAGE_SETUP_INPUTS) {
        variables ="INT32,INT32,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE";
        resp->hdr.type = RTDE_CONTROL_PACKAGE_SETUP_INPUTS;
    } else {
        return -1;
    }

    resp->hdr.size = htons(sizeof(struct rtde_control_package_resp) + variables.length());
    resp->recipe_id = recipe_id & 0xff;
    char *dst = (char *)resp + sizeof(struct rtde_header) + sizeof(uint8_t);
    strncpy(dst, variables.c_str(), variables.length());
    return 1;
}
