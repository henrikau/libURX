#!/bin/bash
set -e

# Note:

# to change priority on the socket, we ned CAP_NET_RAW. To avoid
# entering password, consider letting setcap be passwordless in
# sudoers.d:
#
# --------------------------------------------------------
# cat /etc/sudoers.d/setcap
#
# # Allow henrikau to set capabilities
# henrikau ALL=(root) NOPASSWD:/sbin/setcap
# --------------------------------------------------------

pushd $(dirname $(realpath ${0})) > /dev/null

# Explicitly grant capability to the test-binaries
sudo setcap cap_net_admin,cap_net_raw+eip ../build/test/rtde_handler_test

popd > /dev/null
