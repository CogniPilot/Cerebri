/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <synapse/zbus/channels.h>

#include "casadi/rover.h"
#include "parameters.h"

#define MY_STACK_SIZE 1024
#define MY_PRIORITY 4

enum control_mode_t {
    MODE_INIT = 0,
    MODE_MANUAL = 1,
    MODE_AUTO = 2,
    MODE_CMD_VEL = 3,
};
typedef enum control_mode_t control_mode_t;
char* mode_name[4] = { "init", "manual", "auto", "cmd_vel" };

control_mode_t g_mode = { MODE_INIT };
bool g_armed = false;
static Odometry g_pose = Odometry_init_zero;
static Twist g_cmd_vel = Twist_init_zero;

static void handle_joy(Joy* joy)
{
    // arming
    if (joy->buttons[7] == 1 && !g_armed) {
        if (g_mode == MODE_INIT) {
            printf("Cannot arm until mode selected.\n");
            return;
        }
        printf("Armed in mode: %s\n", mode_name[g_mode]);
        g_armed = true;
    } else if (joy->buttons[6] == 1 && g_armed) {
        printf("Disarmed\n");
        g_armed = false;
        g_mode = MODE_INIT;
    }

    // handle modes
    control_mode_t prev_mode = g_mode;
    if (joy->buttons[0] == 1) {
        g_mode = MODE_MANUAL;
    } else if (joy->buttons[1] == 1) {
        printf("Auto mode rejected: unsupported for diffdrive.\n");
    } else if (joy->buttons[2] == 1) {
        g_mode = MODE_CMD_VEL;
    }

    // notify on mode change
    if (g_mode != prev_mode) {
        printf("Mode changed to: %s!\n", mode_name[g_mode]);
    }

    // translate joystick to twist message
    if (g_mode == MODE_MANUAL) {
        g_cmd_vel.linear.x = joy->axes[1];
        g_cmd_vel.angular.z = joy->axes[3];
    }
}

static void listener_control_diffdrive_callback(const struct zbus_channel* chan)
{
    if (chan == &chan_in_joy) {
        handle_joy((Joy*)(chan->message));
    } else if (chan == &chan_in_odometry) {
        g_pose = *(Odometry*)(chan->message);
    } else if (g_mode == MODE_CMD_VEL && chan == &chan_in_cmd_vel) {
        g_cmd_vel = *(Twist*)(chan->message);
    }
}

ZBUS_LISTENER_DEFINE(listener_control_diffdrive, listener_control_diffdrive_callback);

// computes rc_input from V, omega
void mixer()
{

    // given cmd_vel, compute actuators
    double V = g_cmd_vel.linear.x;
    double omega = g_cmd_vel.angular.z;
    Actuators actuators = Actuators_init_zero;

    // casadi mem args
    casadi_int* iw = NULL;
    casadi_real* w = NULL;
    int mem = 0;

    /* differential_steering:(L,omega,w)->(Vw) */
    {
        double Vw = 0;
        const casadi_real* args[3];
        casadi_real* res[1];
        args[0] = &wheel_base;
        args[1] = &omega;
        args[2] = &wheel_separation;
        res[0] = &Vw;
        differential_steering(args, res, iw, w, mem);
        double omega_fwd = V / wheel_radius;
        double omega_turn = Vw / wheel_radius;
        if (!g_armed) {
            omega_fwd = 0;
            omega_turn = 0;
        }
        actuators.velocity_count = 4;
        actuators.normalized_count = 4;
        actuators.velocity[0] = omega_fwd + omega_turn;
        actuators.velocity[1] = omega_fwd - omega_turn;
        actuators.velocity[2] = omega_fwd - omega_turn;
        actuators.velocity[3] = omega_fwd + omega_turn;
        actuators.normalized[0] = actuators.velocity[0] / max_omega;
        actuators.normalized[1] = actuators.velocity[1] / max_omega;
        actuators.normalized[2] = actuators.velocity[2] / max_omega;
        actuators.normalized[3] = actuators.velocity[3] / max_omega;
    }
    zbus_chan_pub(&chan_out_actuators, &actuators, K_NO_WAIT);
}

void diffdrive_entry_point(void* p1, void* p2, void* p3)
{

    while (true) {
        mixer();

        // sleep to set control rate at 50 Hz
        k_usleep(1e6 / 50);
    }
}

K_THREAD_DEFINE(diffdrive_thread, MY_STACK_SIZE,
    diffdrive_entry_point, NULL, NULL, NULL,
    MY_PRIORITY, 0, 0);

/* vi: ts=4 sw=4 et */
