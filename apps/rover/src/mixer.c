
 
#include <zephyr/kernel.h>

#include "messages.h"

struct msg_actuators_t mixer(struct msg_rc_input_t * msg_rc_input) {
    struct msg_actuators_t actuators_msg;

    enum control_mode_t mode = msg_rc_input->mode;
    double vt = msg_rc_input->thrust;

    double vy = msg_rc_input->yaw;
  
    bool armed = msg_rc_input->armed;
      

    double mix_thrust = armed ? vt : 0;
    double mix_yaw = armed ? vy : 0;

    double scale0 = 1.0;
    if (mode == MODE_MANUAL) {
        scale0 = (1048.0 - -1048.0)/2.0;
    }
    double actuator0 = 1*mix_thrust*scale0 + 0*mix_yaw*scale0;
    if(actuator0 > 1048.0) {
        actuator0 = 1048.0;
    } else if (actuator0 < -1048.0) {
        actuator0 = -1048.0;
    }
    actuators_msg.actuator0_value = actuator0;

double scale1 = 1.0;
    if (mode == MODE_MANUAL) {
        scale1 = (0.3 - -0.3)/2.0;
    }
    double actuator1 =  0*mix_thrust*scale1 +  1*mix_yaw*scale1;
    if(actuator1 > 0.3) {
        actuator1 = 0.3;
    } else if (actuator1 < -0.3) {
        actuator1 = -0.3;
    }
    actuators_msg.actuator1_value = actuator1;



    return actuators_msg;
}
