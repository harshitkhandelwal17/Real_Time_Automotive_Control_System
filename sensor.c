#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "sensor.h"

pthread_t engine_thread;
int thread_created = 0;
int crash_counter = 0;
void* engine_handler(void* arg)
{
	while(shm_ecu->control.ignition){ 
		pthread_mutex_lock(&shm_ecu->lock);
		shm_ecu->sensor.engine_temp = 80 + rand() % 21;
		shm_ecu->sensor.inside_temp = 20 + rand() % 30;
		shm_ecu->sensor.engine_speed = 50 + rand() % 80;
		shm_ecu->sensor.gear_pos = 1 + rand() % 6; 
		shm_ecu->sensor.fuel_level = (float)(1 + rand() %100);
		int ran1 = rand() % 4; // for obstacle detection
		if (ran1 < 3) { // 0, 1, or 2 
		    shm_ecu->sensor.obstacle_detector = 0;
		} else { // 3 
		    shm_ecu->sensor.obstacle_detector = 1;
		}       
		/*if (shm_ecu->sensor.engine_speed > 90) {  // Only above 90 RPM
		    int crash_chance = rand() % 50;
		    if (crash_chance == 0) {
			shm_ecu->sensor.crash = 1;
		    }
		    else shm_ecu->sensor.crash = 0;
		}*/
		shm_ecu->sensor.crash = 0; 
        	crash_counter++;       
        	//check if the counter reached the target interval (10 iterations * 3 sec = 30 sec)
        	if (crash_counter >= 10) {
        	    shm_ecu->sensor.crash = 1; // Set crash status
            	    crash_counter = 0;         // Reset counter
        	}	 
		pthread_mutex_unlock(&shm_ecu->lock);
		sleep(3);
	}
        //printf("[Engine Thread] Ignition turned off. Exiting thread.\n");
	pthread_exit(NULL);
}

void car_status_handler(int sig){
    if (shm_ecu == NULL) return;
	pthread_mutex_lock(&shm_ecu->lock);
	if(sig==SIGUSR1){
		shm_ecu->control.ignition = 1;
		printf("\n[Signal Handler] Car Ignition ON (SIGUSR1)\n");
	}
	if(sig == SIGUSR2){ 
		shm_ecu->control.ignition = 0;
		printf("\n[Signal Handler] Car Ignition OFF (SIGUSR2)\n");
	}
	pthread_mutex_unlock(&shm_ecu->lock);
    
    if(sig == SIGUSR1 && thread_created == 0) {
        if(pthread_create(&engine_thread, NULL, engine_handler, NULL) == 0){
            thread_created = 1;           
        } else {
            perror("\nThread creation failed");
        }
    }
}
 
int main()
{
    printf("--- Car Main Process ---\n");
    printf("Process ID: %d\n",getpid());	
    srand(time(NULL));
    key_t key1 = 9876;	

    int shmid1 = shmget(key1, sizeof(ECU), 0666 | IPC_CREAT);
    if (shmid1 == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    shm_ecu = (ECU *)shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    memset(shm_ecu, 0, sizeof(ECU));
    
    signal(SIGUSR1, car_status_handler);
    signal(SIGUSR2, car_status_handler);

    printf("Waiting for Ignition ON signal (SIGUSR1)...\n");
    
    while(1) {
        pthread_mutex_lock(&shm_ecu->lock);
        
        // CHECK FOR CRASH - IMMEDIATE SHUTDOWN
        if (shm_ecu->sensor.crash == 1) {
            printf("\n[CRASH DETECTED] Initiating emergency shutdown...\n");
            
            // Activate all safety systems
            shm_ecu->control.airbag = 1;
            shm_ecu->control.emergency_stop = 1;
            shm_ecu->control.hazard_lights = 1;
            shm_ecu->control.horn_alarm = 1;
            shm_ecu->control.doors_unlocked = 1;
            shm_ecu->control.ignition = 0;  // TURN OFF ENGINE
            shm_ecu->control.crash_alert_sent = 1; // Flag for server
            
            pthread_mutex_unlock(&shm_ecu->lock);
            sleep(1); // Give time for UI to show crash screen
            break;
        }
        
        // Check if emergency stop button pressed manually
        if (shm_ecu->control.emergency_stop == 1 && shm_ecu->sensor.crash == 0) {
            printf("\n[EMERGENCY STOP] Manual shutdown initiated...\n");
            shm_ecu->control.ignition = 0;
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        pthread_mutex_unlock(&shm_ecu->lock);
        
        // Normal ignition off
        if (thread_created == 1 && shm_ecu->control.ignition == 0) {                 
            if (pthread_join(engine_thread, NULL) == 0) {               
                break;
            } 
        }
        sleep(1); // Check every 1 second (faster response)
    }
    
    // Cleanup
    if (thread_created == 1) {
        pthread_join(engine_thread, NULL);
    }
    
    shmdt(shm_ecu);       
    shmctl(shmid1, IPC_RMID, NULL);         
    printf("--- Car Main Process Exiting ---\n");

    return 0;
}
