#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include "sensor.h"

pthread_t engine_thread;
int thread_created = 0;
int crash_counter = 0;
int log_fd = -1;

void write_log(const char* message) {
    if (log_fd == -1) return;
    
    time_t now = time(NULL);
    char timestamp[64];
    struct tm *t = localtime(&now);
    snprintf(timestamp, sizeof(timestamp), "[%04d-%02d-%02d %02d:%02d:%02d] ",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    
    write(log_fd, timestamp, strlen(timestamp));
    write(log_fd, message, strlen(message));
    write(log_fd, "\n", 1);
}

void* engine_handler(void* arg)
{
    write_log("Engine handler thread started");
    
    while(shm_ecu->control.ignition){ 
        pthread_mutex_lock(&shm_ecu->lock);
        shm_ecu->sensor.engine_temp = 80 + rand() % 21;
        shm_ecu->sensor.inside_temp = 20 + rand() % 30;
        shm_ecu->sensor.engine_speed = 50 + rand() % 80;
        shm_ecu->sensor.gear_pos = 1 + rand() % 6; 
        shm_ecu->sensor.fuel_level = (float)(1 + rand() %100);
        
        int ran1 = rand() % 4;
        if (ran1 < 3) {
            shm_ecu->sensor.obstacle_detector = 0;
        } else {
            shm_ecu->sensor.obstacle_detector = 1;
            write_log("Obstacle detected!");
        }
        
        shm_ecu->sensor.crash = 0; 
        crash_counter++;
        
        if (crash_counter >= 10) {
            shm_ecu->sensor.crash = 1;
            write_log("CRASH DETECTED - Emergency protocols initiated");
            crash_counter = 0;
        }
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), 
                 "Sensors: Temp=%f°C, Speed=%fRPM, Gear=%d, Fuel=%.1f%%",
                 shm_ecu->sensor.engine_temp, shm_ecu->sensor.engine_speed,
                 shm_ecu->sensor.gear_pos, shm_ecu->sensor.fuel_level);
        write_log(log_msg);
        
        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Engine handler thread exiting - ignition off");
    pthread_exit(NULL);
}

void car_status_handler(int sig){
    if (shm_ecu == NULL) return;
    
    pthread_mutex_lock(&shm_ecu->lock);
    if(sig==SIGUSR1){
        shm_ecu->control.ignition = 1;
        printf("\n[Signal Handler] Car Ignition ON (SIGUSR1)\n");
        write_log("Car Ignition turned ON (SIGUSR1 received)");
    }
    if(sig == SIGUSR2){ 
        shm_ecu->control.ignition = 0;
        printf("\n[Signal Handler] Car Ignition OFF (SIGUSR2)\n");
        write_log("Car Ignition turned OFF (SIGUSR2 received)");
    }
    pthread_mutex_unlock(&shm_ecu->lock);
    
    if(sig == SIGUSR1 && thread_created == 0) {
        if(pthread_create(&engine_thread, NULL, engine_handler, NULL) == 0){
            thread_created = 1;
            write_log("Engine thread created successfully");
        } else {
            perror("\nThread creation failed");
            write_log("ERROR: Engine thread creation failed");
        }
    }
}
 
int main()
{
    printf("--- Car Main Process ---\n");
    
    // Open log file
    log_fd = open("sensor.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Failed to open log file");
    } else {
        write_log("========== Car Main Process Started ==========");
    }
    
    pid_t current_pid = getpid();
    printf("Process ID: %d\n", current_pid);
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Car process started with PID: %d", current_pid);
    write_log(log_msg);
    
    srand(time(NULL));
    key_t key1 = 9876;	

    int shmid1 = shmget(key1, sizeof(ECU), 0666 | IPC_CREAT);
    if (shmid1 == -1) {
        perror("shmget failed");
        write_log("ERROR: Shared memory creation failed");
        exit(1);
    }
    
    shm_ecu = (ECU *)shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU *)-1) {
        perror("shmat failed");
        write_log("ERROR: Shared memory attach failed");
        exit(1);
    }
    
    write_log("Shared memory initialized successfully");
    
    memset(shm_ecu, 0, sizeof(ECU));
    pthread_mutex_init(&shm_ecu->lock, NULL);

    shm_ecu->pid = current_pid;
    printf("Car PID %d stored in Shared Memory.\n", current_pid);
    
    signal(SIGUSR1, car_status_handler);
    signal(SIGUSR2, car_status_handler);

    printf("Waiting for Ignition ON signal (SIGUSR1)...\n");
    write_log("Waiting for ignition ON signal...");
    
    while(1) {
        pthread_mutex_lock(&shm_ecu->lock);
        
        if (shm_ecu->sensor.crash == 1) {
            printf("\n[CRASH DETECTED] Initiating emergency shutdown...\n");
            write_log("EMERGENCY: Crash detected - activating all safety systems");
            
            shm_ecu->control.airbag = 1;
            shm_ecu->control.emergency_stop = 1;
            shm_ecu->control.hazard_lights = 1;
            shm_ecu->control.horn_alarm = 1;
            shm_ecu->control.doors_unlocked = 1;
            shm_ecu->control.ignition = 0;
            shm_ecu->control.crash_alert_sent = 1;
            
            write_log("Safety systems activated: airbag, emergency stop, hazard lights, horn, doors unlocked");
            
            pthread_mutex_unlock(&shm_ecu->lock);
            sleep(1);
            break;
        }
        
        if (shm_ecu->control.emergency_stop == 1 && shm_ecu->sensor.crash == 0) {
            printf("\n[EMERGENCY STOP] Manual shutdown initiated...\n");
            write_log("Manual emergency stop initiated");
            shm_ecu->control.ignition = 0;
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        pthread_mutex_unlock(&shm_ecu->lock);
        
        if (thread_created == 1 && shm_ecu->control.ignition == 0) {                 
            if (pthread_join(engine_thread, NULL) == 0) {
                write_log("Engine thread joined successfully");
                break;
            } 
        }
        sleep(1);
    }
    
    // Cleanup
    if (thread_created == 1) {
        pthread_join(engine_thread, NULL);
    }
    
    shmdt(shm_ecu);
    shmctl(shmid1, IPC_RMID, NULL);
    
    write_log("Shared memory detached and cleaned up");
    write_log("========== Car Main Process Exiting ==========");
    
    printf("--- Car Main Process Exiting ---\n");
    
    if (log_fd != -1) {
        close(log_fd);
    }

    return 0;
}
