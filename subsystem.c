#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include "sensor.h"

// === Controller Threads === //
void* fan_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        if (shm_ecu->sensor.engine_temp > 80.0)
            shm_ecu->control.fan_status = 1;
        else
            shm_ecu->control.fan_status = 0;

        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}

void* ac_controller(void* arg){
	while(1){
		pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        if (shm_ecu->sensor.inside_temp > 24.0)
            shm_ecu->control.ac_control = 1;
        else
            shm_ecu->control.ac_control = 0;

        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}
void* brake_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        if (shm_ecu->sensor.obstacle_detector == 1 || shm_ecu->sensor.engine_speed > 100)
            shm_ecu->control.brake_status = 1;
            if(shm_ecu->control.brake_status && shm_ecu->sensor.engine_speed < 40){            	
            		shm_ecu->sensor.engine_speed -= 20;
            }	
        else
            shm_ecu->control.brake_status = 0;

        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}

void* light_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
	shm_ecu->control.back_light = (shm_ecu->sensor.gear_pos == 6) ? 1 : 0;  
	shm_ecu->control.reverse_camera = (shm_ecu->sensor.gear_pos == 6) ? 1 : 0;    
        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}

void* safety_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        if (shm_ecu->sensor.crash == 1) {
            shm_ecu->control.emergency_stop = 1;
            shm_ecu->control.airbag = 1;
        } else {
            shm_ecu->control.emergency_stop = 0;
            shm_ecu->control.airbag = 0;
        }
        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}
void* fuel_controller(void* arg){
	while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        if (shm_ecu->sensor.fuel_level == 100) {
            shm_ecu->control.fuel_status = 1;
        } else if(shm_ecu->sensor.fuel_level < 100 || shm_ecu->sensor.fuel_level > 25) {
            shm_ecu->control.fuel_status = 0;
        } else{
        	shm_ecu->control.fuel_status = -1;
        }
        pthread_mutex_unlock(&shm_ecu->lock);
        //usleep(500000);
        sleep(3);
    }
    return NULL;
}
int main() {
    printf("--- Subsystem Process ---\n");

    key_t key1 = 2345;
    int shmid1 = shmget(key1, sizeof(ECU), 0666);
    if (shmid1 == -1) {
        perror("shmget failed. Is sensor.c running?");
        return 1;
    }

    shm_ecu = (ECU*) shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        return 1;
    }

    printf("Waiting for ignition ON...\n");
    while (shm_ecu->control.ignition == 0) sleep(1);

    printf("Ignition ON. Starting controllers...\n");

    pthread_t fan_thread, brake_thread, light_thread, safety_thread;
    pthread_create(&fan_thread, NULL, fan_controller, NULL);
    pthread_create(&brake_thread, NULL, brake_controller, NULL);
    pthread_create(&light_thread, NULL, light_controller, NULL);
    pthread_create(&safety_thread, NULL, safety_controller, NULL);

    pthread_join(fan_thread, NULL);
    pthread_join(brake_thread, NULL);
    pthread_join(light_thread, NULL);
    pthread_join(safety_thread, NULL);

    printf("Ignition OFF. Controllers stopped.\n");
    shmdt(shm_ecu);
    return 0;
}
