#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

int main(){
    printf("--- Control Panel Process ---\n");
    
    pid_t car_pid;
    int status;
    
    printf("Enter Car PID (Process ID): ");
    if(scanf("%d",&car_pid) != 1 || car_pid <= 0){
        fprintf(stderr, "Invalid PID entered.\n");
        return 1;
    }
    
    printf("\nPress 1 to turn ON ignition: ");
    if(scanf("%d",&status) == 1 && status == 1){
        if(kill(car_pid, SIGUSR1) == 0){
            printf("Sent SIGUSR1 to PID %d (Ignition ON).\n", car_pid);
        } else {
            perror("Error sending SIGUSR1");
            return 1;
        }
    } else {
        printf("Ignition not turned ON. Exiting.\n");
        return 0;
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
