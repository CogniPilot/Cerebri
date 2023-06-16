/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock.pb.h"
#include <zephyr/kernel.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <soc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <pb_decode.h>
#include <pb_encode.h>
#include <synapse_protobuf/actuators.pb.h>

#include <synapse_tinyframe/SynapseTopics.h>
#include <synapse_tinyframe/TinyFrame.h>
#include <synapse_tinyframe/utils.h>

#include <synapse/zbus/channels.h>

#include <zephyr/sys/ring_buffer.h>

#define MY_STACK_SIZE 500
#define MY_PRIORITY -10
#define BIND_PORT 4241
#define RX_BUF_SIZE 1024

int64_t connect_time = 0;
Clock g_sim_clock = Clock_init_default;
pthread_mutex_t g_lock_sim_clock;
static int serv = 0;
int client = 0;

static void write_sim(TinyFrame* tf, const uint8_t* buf, uint32_t len)
{
    int client = *(int*)(tf->userdata);
    if (len > 0) {
        send(client, buf, len, 0);
    }
}

static TinyFrame g_tf = {
    .peer_bit = TF_MASTER,
    .write = write_sim,
    .userdata = &client,
};

pthread_t thread1;

void listener_dream_sitl_callback(const struct zbus_channel* chan)
{
    if (chan == &chan_out_actuators) {
        TF_Msg msg;
        TF_ClearMsg(&msg);
        uint8_t buf[500];
        pb_ostream_t stream = pb_ostream_from_buffer((pu8)buf, sizeof(buf));
        int status = pb_encode(&stream, Actuators_fields, chan->message);
        if (status) {
            msg.type = SYNAPSE_OUT_ACTUATORS_TOPIC;
            msg.data = buf;
            msg.len = stream.bytes_written;
            TF_Send(&g_tf, &msg);
        } else {
            printf("dream_sitl: encoding failed: %s\n", PB_GET_ERROR(&stream));
        }
    }
}
ZBUS_LISTENER_DEFINE(listener_dream_sitl, listener_dream_sitl_callback);

static TF_Result sim_clock_listener(TinyFrame* tf, TF_Msg* frame)
{
    Clock msg = Clock_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(frame->data, frame->len);
    int status = pb_decode(&stream, Clock_fields, &msg);
    if (status) {
        pthread_mutex_lock(&g_lock_sim_clock);
        g_sim_clock = msg;
        pthread_mutex_unlock(&g_lock_sim_clock);
    } else {
        printf("dream_sitl: decoding failed: %s\n", PB_GET_ERROR(&stream));
    }
    return TF_STAY;
}

TF_Result generic_listener(TinyFrame* tf, TF_Msg* frame)
{
    return TF_STAY;
}

void* native_sim_entry_point(void* data)
{
    printf("dream_sitl: sim core running\n");

    // setup tinyframe
    TF_AddGenericListener(&g_tf, generic_listener);
    TF_AddTypeListener(&g_tf, SYNAPSE_SIM_CLOCK_TOPIC, sim_clock_listener);

    struct sockaddr_in bind_addr;
    static int counter;

    serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv < 0) {
        printf("dream_sitl: error: socket: %d\n", errno);
        exit(1);
    }

    int status = fcntl(serv, F_SETFL, fcntl(serv, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1) {
        perror("calling fcntrl");
        exit(1);
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(BIND_PORT);

    if (bind(serv, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        printf("dream_sitl: bind() failed: %d\n", errno);
        exit(1);
    }

    if (listen(serv, 5) < 0) {
        printf("dream_sitl: error: listen: %d\n", errno);
        exit(1);
    }

    printf("dream_sitl: listening to server on port: %d\n", BIND_PORT);

    struct timespec remaining, request;

    printf("dream_sitl: waiting for client connection\n");
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        char addr_str[32];
        client = accept(serv, (struct sockaddr*)&client_addr,
            &client_addr_len);
        fcntl(client, F_SETFL, O_NONBLOCK);

        if (client < 0) {
            request.tv_sec = 1;
            request.tv_nsec = 0;
            nanosleep(&request, &remaining);
            continue;
        }

        inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
            addr_str, sizeof(addr_str));
        printf("dream_sitl: connection #%d from %s\n", counter++, addr_str);

        while (true) {
            // write received data to sim_rx_buf
            uint8_t data[RX_BUF_SIZE];
            int len = recv(client, data, RX_BUF_SIZE, 0);
            if (len > 0) {
                TF_Accept(&g_tf, data, len);
            }
            request.tv_sec = 0;
            request.tv_nsec = 1000000; // 1 ms
            nanosleep(&request, &remaining);
        }
    }
}

void native_sim_start_task(void)
{
    pthread_create(&thread1, NULL, native_sim_entry_point, NULL);
}

void native_sim_stop_task(void)
{
    pthread_join(thread1, NULL);
}

static void zephyr_sim_entry_point(void)
{
    printf("dream_sitl: zephyr sim entry point\n");
    while (true) {
        int64_t uptime = k_uptime_get();
        int64_t sec = uptime / 1.0e3;
        int32_t nsec = (uptime - sec * 1e3) * 1e6;
        Clock sim_clock;

        // fast forward zephyClock time to match sim
        pthread_mutex_lock(&g_lock_sim_clock);
        sim_clock = g_sim_clock;
        pthread_mutex_unlock(&g_lock_sim_clock);

        int64_t delta_sec = sim_clock.sim.sec - sec;
        int32_t delta_nsec = sim_clock.sim.nsec - nsec;
        int64_t wait_msec = delta_sec * 1e3 + delta_nsec * 1e-6;

        if (wait_msec > 0) {
            // printf("sim: sec %ld nsec %d\n", sim_clock.sim.sec, sim_clock.sim.nsec);
            // printf("uptime: sec %ld nsec %d\n", sec, nsec);
            // printf("wait: msec %ld\n", wait_msec);
            k_msleep(wait_msec);
        } else {
            struct timespec request, remaining;
            request.tv_sec = 0;
            request.tv_nsec = 1000000;
            nanosleep(&request, &remaining);
        }
    }
}

// native tasks
NATIVE_TASK(native_sim_start_task, PRE_BOOT_1, 0);
NATIVE_TASK(native_sim_stop_task, ON_EXIT, 0);

// zephyr threads
K_THREAD_DEFINE(zephyr_sim_thread, MY_STACK_SIZE, zephyr_sim_entry_point,
    NULL, NULL, NULL, MY_PRIORITY, 0, 0);

// vi: ts=4 sw=4 et
