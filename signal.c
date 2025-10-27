// signal.c (Modified)
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include <sys/shm.h> // Include for shmget/shmat/shmdt
#include "sensor.h"

// shm_ecu definition is assumed to be in sensor.h or globally defined in the project setup
// For standalone compilation, you might need: ECU *shm_ecu;

pid_t get_car_pid_from_shm() {
    key_t key = 9876;
    int shmid = shmget(key, sizeof(ECU), 0666);
    if (shmid == -1) {
        perror("shmget failed (Is sensor.c running?)");
        return -1;
    }
    // Corrected to use a local pointer for reading, as shm_ecu is likely needed globally
    // in sensor.c, but here we just need the PID.
    ECU *local_shm_ecu = (ECU*) shmat(shmid, NULL, 0);
    if (local_shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        return -1;
    }
    
    pid_t car_pid = local_shm_ecu->pid;
    
    shmdt(local_shm_ecu);
    
    return car_pid;
}

int main(){
    printf("--- Control Panel Process ---\n");
    
    pid_t car_pid = get_car_pid_from_shm();
    int status;
    
    // sleep(1) is removed as it's not strictly necessary and can be confusing
    
    if (car_pid <= 0) {
        fprintf(stderr, "Invalid Car PID from Shared Memory. Is sensor.c running and initialized?\n");
        return 1;
    }
    
    if(kill(car_pid, SIGUSR1) == 0){
        printf("\nSent SIGUSR1 to PID %d (Ignition ON).\n", car_pid);
    } else {        
        perror("Error sending SIGUSR1");        
        return 1;
    }
    
    // Updated instructions for the user
    printf("\n------------------------------------------------------------\n");
    printf("   PID %d is running the Car simulation (sensor.c).\n", car_pid);
    printf("   To **CRASH** the car, open another terminal and run:\n");
    printf("      $ kill -2 %d\n", car_pid);
    printf("------------------------------------------------------------\n");
    
    printf("\nPress 0 to turn OFF ignition and exit: ");
    if(scanf("%d",&status) == 1 && status == 0){
        if(kill(car_pid, SIGUSR2) == 0){
            printf("Sent SIGUSR2 to PID %d (Ignition OFF).\n", car_pid);
        } else {
            perror("Error sending SIGUSR2");
            return 1;
        }
    } else {
        printf("Ignition remains ON. Exiting control panel.\n");
    }
    
    printf("--- Control Panel Exiting ---\n");
    return 0;
}
