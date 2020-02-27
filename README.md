# liburx - library for connecting to a Universal Robots Controller

## liburx

The goal is to make it easy to create a Real-Time capable controller
loop running on a small-ish system. UR provides a python interface for
connecting to the robot, but this does not connect to the RTDE interface
which is capable of 500Hz updates (on the e-series).

liburx contains all required functionality for setting up an rtde
interface and work to to UR-(3|5|10) specific things is ongoing.

### Recipes

When communicating with the UR controller, the data that flows via the
RTDE protocol is managed by socalled "recipes". A recipe will specify
which variables are of interest, and each variable has a pre-determined
datatype. To make the process a bit easier, a RTDE_Recipe will determine
this for you, but you still have to provide it with a memory reference
into where it can store the incoming data.

Example; a timestamp is of type DOUBLE (8 bytes). If this is the only
variable you are interested in, you add a single field to a recipe
("timestamp") and provide a reference to a double. For each new
data-packet RTDE_Handler now receives, the local timestamp-variable will
be updated. This makes it easy to retrieve and manage the data. See a
more elaborate example below.

Warning: if you decide to provide the recipe with a 1 byte unsigned char
for the double, the parser will *happily* write 8 bytes into this location.

See
[real-time-data-exchange-rtde-guide-22229](https://www.universal-robots.com/how-tos-and-faqs/how-to/ur-how-tos/real-time-data-exchange-rtde-guide-22229/
"RTDE Guide - 22229") for a complete list of available variables. This
list is also available in [magic.h](include/urx/magic.h)

## Examples

Example code can be found in the tools/ subdirectory

### get_remote

Very simple script that will connect to a controller and query version
running. It will then query the controller for timestamp and joint
positions.


## Using liburx


### Using RTDE_Handler to poll internal state in the robot
Only a few steps are required to connect and start retrieving data. For
a more elaborate example, see [get_remote.cpp](tools/get_remote.cpp)

``` c++
#include <urx/rtde_handler.hpp>
#include <urx/rtde_recipe.hpp>

void robot()
{
	urx::RTDE_Handler h("10.0.2.4");
	h.connect_ur();
	if (!h.set_version())
		return -1;

	double ts;
	double actual_q[6];
	double tq[6];
	urx::RTDE_Recipe r = urx::RTDE_Recipe();
	r.AddField("timestamp", &ts);
	r.AddField("actual_q", actual_q);
	r.AddField("target_q", tq);
	if (!h.register_output_recipe(&r))
		return -1;

	h.start();
	while (1) {
		// wait for data
		h.recv();
		std::cout << ts << std::endl;
		// do stuff with actual_q and target_q
		// ...
	}
	h.stop();
}
```

### Using urx::Robot() to control position using joint speed using URScript and a tight RTDE Feedback loop
See [robot_speed.cpp](tools/robot_speed.cpp)

``` c++
#include <urx/robot.hpp>
#include <urx/helper.hpp>
#include <unistd.h>


double new_speed(const double err, const double prev_err, const double prev_speed)
{
    constexpr double k_p = 0.7;
    constexpr double k_i = 0.005;
    constexpr double k_d = 1.1;

    double p = k_p * err;
    double i = k_i *  prev_speed; // accumlated speed, not really summed error
    double d = (err - prev_err * k_d)/k_d;
    return  p + i + d;
}

int main(int argc, char *argv[])
{
    // target position for joints
    std::vector<double> aq(urx::DOF);
    aq[urx::BASE]     = urx::deg_to_rad( 180.0);
    aq[urx::SHOULDER] = urx::deg_to_rad(-135.0);
    aq[urx::ELBOW]    = urx::deg_to_rad(  90.0);
    aq[urx::W1]       = urx::deg_to_rad( -90.0);
    aq[urx::W2]       = urx::deg_to_rad(  90.0);
    aq[urx::W3]       = urx::deg_to_rad( -90.0);

    std::atomic<bool> running = true;
    std::vector<double> w(urx::DOF);;
    std::vector<double> prev_err(urx::DOF);

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " IP path/to/script" << std::endl;
        return 1;
    }

    // Robot startup
    urx::Robot robot = urx::Robot(argv[1]);
    if (!robot.init_output() ||
        !robot.init_input()  ||
        !robot.upload_script(argv[2])) {
        std::cout << "urx::Robot init failed" << std::endl;
        return 2;
    }
    if (!robot.start()) {
        std::cout << "urx::Robot startup failed" << std::endl;
        return 3;
    }

    while (running) {
        urx::Robot_State state = robot.state();
        for (std::size_t i = 0; i < urx::DOF; ++i) {
            double err = aq[i] - state.jq[i];
            w[i] = new_speed(err, prev_err[i], w[i]);
            prev_err[i] = err;
        }

        // if all errors are ok, we are done
        if (close_vec(state.jq, aq, 0.0001)) {
            running = false;
            for (std::size_t i = 0; i < urx::DOF; ++i)
                w[i] = 0.0;
        }
        robot.update_w(w);
    }

    robot.stop();
    return 0;
}

```
