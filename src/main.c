#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define _GNU_SOURCE
#include <unistd.h>
pid_t gettid(void);

#include "ftd2xx.h"

static unsigned char p = 0x00;

FT_HANDLE ftHandle;

static void handler(int sig, siginfo_t *si, void *uc) {
    DWORD nbw = 0;

    if (FT_Write(ftHandle, &p, 1, &nbw) == FT_OK) {
        //if (nbw) p >>= 1; divide by two until you reach 0
    }
}

void timerinit() {
    int res = 0;
    timer_t timerId = 0;

    printf("Setting up timer...\n");

    /*specifies the notification signal (SIGRTMIN)*/
    struct sigevent sev = {.sigev_notify = SIGEV_SIGNAL,
                           .sigev_signo = SIGRTMIN};

    /* specifies the action when receiving a signal */
    struct sigaction sa = {.sa_flags = SA_SIGINFO,
                           .sa_sigaction = handler};

    /* specify start delay and interval */
    struct itimerspec its = {.it_value.tv_sec = 1,  // start the timer after 1 second
                             .it_value.tv_nsec = 0,
                             .it_interval.tv_sec = 0,
                             .it_interval.tv_nsec = 0.02 * 1e9};  // call the handler function 50 times per second

    printf("Signal Interrupt Timer - thread-id: %d\n", gettid());

    /* create timer */
    res = timer_create(CLOCK_REALTIME, &sev, &timerId);

    if (res != 0) {
        fprintf(stderr, "Error timer_create: %s\n", strerror(errno));
        exit(-1);
    }

    /* Initialize signal */
    sigemptyset(&sa.sa_mask);

    printf("Establishing handler for signal SIGRTMIN\n");

    /* Register signal handler */
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        fprintf(stderr, "Error sigaction: %s\n", strerror(errno));
        exit(-1);
    }

    /* start timer */
    res = timer_settime(timerId, 0, &its, NULL);

    if (res != 0) {
        fprintf(stderr, "Error timer_settime: %s\n", strerror(errno));
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    DWORD baudRate = 256000;
    FT_STATUS ftStatus = FT_OK;

    int portNumber;

    portNumber = 0;

    printf("Connecting to FT232H chip...\n");
    ftStatus = FT_Open(portNumber, &ftHandle);
    if (ftStatus != FT_OK) {
        /* FT_Open can fail if the ftdi_sio module is already loaded. */
        printf("FT_Open(%d) failed (error %d).\n", portNumber, (int)ftStatus);
        printf("Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
        printf("If so, unload them using rmmod, as they conflict with ftd2xx.\n");
        return 1;
    }

    /* Enable bit-bang mode, where 8 UART pins (RX, TX, RTS etc.) become
	 * general-purpose I/O pins.
	 */
    printf("Selecting asynchronous bit-bang mode.\n");
    ftStatus = FT_SetBitMode(ftHandle,
                             0xFF, /* sets all 8 pins as outputs */
                             FT_BITMODE_ASYNC_BITBANG);
    if (ftStatus != FT_OK) {
        printf("FT_SetBitMode failed (error %d).\n", (int)ftStatus);
        goto exit;
    }

    /* In bit-bang mode, setting the baud rate gives a clock rate
	 * 16 times higher, e.g. baud = 9600 gives 153600 bytes per second.
	 */
    printf("Setting clock rate to %d\n", baudRate * 16);
    ftStatus = FT_SetBaudRate(ftHandle, baudRate);
    if (ftStatus != FT_OK) {
        printf("FT_SetBaudRate failed (error %d).\n", (int)ftStatus);
        goto exit;
    }

    timerinit();

    printf("Enter an unsigned 8-bit integer\n");

    while (scanf(" %hhu", &p))
        ;

exit:
    /* Return chip to default (UART) mode. */
    (void)FT_SetBitMode(ftHandle,
                        0, /* ignored with FT_BITMODE_RESET */
                        FT_BITMODE_RESET);

    (void)FT_Close(ftHandle);
    return 0;
}
