/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mixing.h"

void rdd2_set_actuators(synapse_msgs_Actuators* msg, double roll, double pitch, double yaw, double thrust)
{
    msg->has_header = true;
    stamp_header(&msg->header, k_uptime_ticks());
    msg->header.seq++;
    strncpy(msg->header.frame_id, "odom", sizeof(msg->header.frame_id) - 1);

    const float k = 10;

    msg->position_count = 0;
    msg->velocity_count = 4;
    msg->normalized_count = 0;
    msg->velocity[0] = k * (thrust + pitch - roll + yaw);
    msg->velocity[1] = k * (thrust - pitch + roll + yaw);
    msg->velocity[2] = k * (thrust + pitch + roll - yaw);
    msg->velocity[3] = k * (thrust - pitch - roll - yaw);
}

/* vi: ts=4 sw=4 et */
