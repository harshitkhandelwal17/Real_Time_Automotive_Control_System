#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include "sensor.h"


pid_t get_car_pid_from_shm() {
    key_t key = 9876;
    int shmid = shmget(key, sizeof(ECU), 0666);
    if (shmid == -1) {
        perror("shmget failed (Is sensor.c running?)");
        return -1;
    }
    shm_ecu = (ECU*) shmat(shmid, NULL, 0);
    if (shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        return -1;
    }
   
    pid_t car_pid = shm_ecu->pid;
    
    shmdt(shm_ecu);
    
    return car_pid;
}

int main(){
    printf("--- Control Panel Process ---\n");
    
    pid_t car_pid = get_car_pid_from_shm();
    int status;
       
    sleep(1);
    
    if (car_pid <= 0) {
        fprintf(stderr, "Invalid Car PID from Shared Memory.\n");
        return 1;
    }
    
    if(kill(car_pid, SIGUSR1) == 0){
        printf("\nSent SIGUSR1 to PID %d (Ignition ON).\n", car_pid);
    } else {       
        perror("Error sending SIGUSR1");       
    	return 1;
    }
    
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
