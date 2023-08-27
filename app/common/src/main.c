/*
 * Copyright (c) 2023 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ARCH_POSIX)
#include <signal.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_CEREBRI_BOOT_BANNER)
const char banner_brain[] = "                            \033[0m\033[38;5;252m              ▄▄▄▄▄▄▄▄\n"
                            "\033[2;34m         ▄▄▄▄▄ \033[2;33m▄▄▄▄▄\033[0m\033[38;5;252m                    ▀▀▀▀▀▀▀▀▀\n"
                            "\033[2;34m     ▄███████▀\033[2;33m▄██████▄\033[0m\033[38;5;252m   ▀█████████████████████▀\n"
                            "\033[2;34m  ▄██████████ \033[2;33m████████\033[31m ▄\033[0m\033[38;5;249m   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄\n"
                            "\033[2;34m ███████████▀ \033[2;33m███████▀\033[31m ██\033[0m\033[38;5;249m   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n"
                            "\033[2;34m█████████▀   \033[2;33m▀▀▀▀▀▀▀▀\033[31m ████\033[0m\033[38;5;246m   ▀███████████▀\n"
                            "\033[2;34m▀█████▀ \033[2;32m▄▄███████████▄\033[31m ████\033[0m\033[38;5;243m   ▄▄▄▄▄▄▄▄▄\n"
                            "\033[2;34m  ▀▀▀ \033[2;32m███████████████▀\033[31m ████\033[0m\033[38;5;243m   ▀▀▀▀▀▀▀▀\n"
                            "       \033[2;32m▀▀█████▀▀▀▀▀▀\033[31m  ▀▀▀▀\033[0m\033[38;5;240m   ▄█████▀\n"
                            "              \033[2;90m ████████▀    ▄▄▄\n"
                            "              \033[2;90m ▀███▀       ▀▀▀\n"
                            "              \033[2;90m  ▀▀      \033[0m\n";
const char banner_name[] = "╔═══╗╔═══╗╔═══╗╔═╗ ╔╗╔══╗╔═══╗╔══╗╔╗   ╔═══╗╔════╗\n"
                           "║╔═╗║║╔═╗║║╔═╗║║║║ ║║╚╣╠╝║╔═╗║╚╣╠╝║║   ║╔═╗║║╔╗╔╗║\n"
                           "║║ ╚╝║║ ║║║║ ╚╝║║╚╗║║ ║║ ║║ ║║ ║║ ║║   ║║ ║║╚╝║║╚╝\n"
                           "║║   ║║ ║║║║╔═╗║╔╗╚╝║ ║║ ║╚═╝║ ║║ ║║   ║║ ║║  ║║  \n"
                           "║║ ╔╗║║ ║║║║╚╗║║║╚╗║║ ║║ ║╔══╝ ║║ ║║ ╔╗║║ ║║  ║║  \n"
                           "║╚═╝║║╚═╝║║╚═╝║║║ ║║║╔╣╠╗║║   ╔╣╠╗║╚═╝║║╚═╝║ ╔╝╚╗ \n"
                           "╚═══╝╚═══╝╚═══╝╚╝ ╚═╝╚══╝╚╝   ╚══╝╚═══╝╚═══╝ ╚══╝ \n\033[31m"
                           "       ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━┓ ┏━━━┓┏━━┓\n"
                           "       ┃┏━┓┃┃┏━━┛┃┏━┓┃┃┏━━┛┃┏┓┃ ┃┏━┓┃┗┫┣┛\n"
                           "       ┃┃ ┗┛┃┗━┓ ┃┗━┛┃┃┗━┓ ┃┗┛┗┓┃┗━┛┃ ┃┃ \n"
                           "       ┃┃ ┏┓┃┏━┛ ┃┏┓┏┛┃┏━┛ ┃┏━┓┃┃┏┓┏┛ ┃┃ \n"
                           "       ┃┗━┛┃┃┗━━┓┃┃┃┗┓┃┗━━┓┃┗━┛┃┃┃┃┗┓┏┫┣┓\n"
                           "       ┗━━━┛┗━━━┛┗┛┗━┛┗━━━┛┗━━━┛┗┛┗━┛┗━━┛\n\033[0m";
#endif

LOG_MODULE_REGISTER(main, CONFIG_CEREBRI_LOG_LEVEL);

static volatile int keepRunning = 1;

#if defined(CONFIG_ARCH_POSIX)
void intHandler(int dummy)
{
    (void)dummy;
    keepRunning = 0;
    LOG_INF("sigint caught");
    exit(0);
}
#endif

int main(void)
{
#if defined(CONFIG_CEREBRI_BOOT_BANNER)
    printf("%s%s\n", banner_brain, banner_name);
#endif
    LOG_INF("Cerebri %d.%d.%d", CONFIG_CEREBRI_VERSION_MAJOR, CONFIG_CEREBRI_VERSION_MINOR, CONFIG_CEREBRI_VERSION_PATCH);

#if defined(CONFIG_ARCH_POSIX)
    signal(SIGINT, intHandler);
    while (keepRunning) {
        k_msleep(1000);
    }
#endif
    return 0;
}

// vi: ts=4 sw=4 et
