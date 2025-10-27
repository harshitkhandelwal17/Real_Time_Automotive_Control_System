// signal.c (Updated to forward SIGINT)
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include <sys/shm.h>
#include "sensor.h"

// Variable to store the PID of the car process globally for the signal handler
pid_t car_pid_global = -1;

pid_t get_car_pid_from_shm() {
    key_t key = 9876;
    int shmid = shmget(key, sizeof(ECU), 0666);
    if (shmid == -1) {
        perror("shmget failed (Is sensor.c running?)");
        return -1;
    }
    ECU *local_shm_ecu = (ECU*) shmat(shmid, NULL, 0);
    if (local_shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        return -1;
    }
    
    pid_t car_pid = local_shm_ecu->pid;
    
    shmdt(local_shm_ecu);
    
    return car_pid;
}

// Handler for Ctrl+C (SIGINT) in the Control Panel
void control_panel_crash_handler(int sig) {
    if (car_pid_global > 0) {
        printf("\n[Control Panel] Forwarding SIGINT (Crash) to Car PID %d...\n", car_pid_global);
        if (kill(car_pid_global, SIGINT) == 0) {
            printf("[Control Panel] Sent SIGINT successfully. Car is crashing.\n");
        } else {
            perror("[Control Panel] Error forwarding SIGINT");
        }
    }
    // Re-raise the signal to terminate the control panel itself after forwarding
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

int main(){
    printf("--- Control Panel Process ---\n");
    
    car_pid_global = get_car_pid_from_shm();
    int status = -1;
    
    if (car_pid_global <= 0) {
        fprintf(stderr, "Invalid Car PID from Shared Memory. Is sensor.c running and initialized?\n");
        return 1;
    }

    // Register the SIGINT handler
    signal(SIGINT, control_panel_crash_handler);
    
    // 1. Send Ignition ON
    if(kill(car_pid_global, SIGUSR1) == 0){
        printf("\nSent SIGUSR1 to PID %d (Ignition ON).\n", car_pid_global);
    } else {        
        perror("Error sending SIGUSR1");        
        return 1;
    }
    
    printf("\n------------------------------------------------------------\n");
    printf("   Car Process PID: %d\n", car_pid_global);
    printf("   Enter **0** to turn OFF ignition and exit.\n");
    printf("   Press **Ctrl+C** to trigger a **CRASH**.\n");
    printf("------------------------------------------------------------\n");
    
    // 2. Wait for User Input
    printf("\nAction: ");
    if(scanf("%d",&status) == 1){
        if(status == 0){
            // Send Ignition OFF
            if(kill(car_pid_global, SIGUSR2) == 0){
                printf("Sent SIGUSR2 to PID %d (Ignition OFF).\n", car_pid_global);
            } else {
                perror("Error sending SIGUSR2");
                return 1;
            }
        } else {
            printf("Invalid command (%d). Ignition remains ON. Exiting...\n", status);
        }
    } else {
        printf("Invalid input. Ignition remains ON. Exiting...\n");
    }
    
    printf("--- Control Panel Exiting ---\n");
    return 0;
}
