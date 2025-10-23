#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

typedef struct ecu_sensor{
	float engine_temp; //random
	float engine_speed; //random
	int obstacle_detector; // 0/1
	int gear_pos; //1-6
	float fuel_level; //0 to 100
	int seatbelt; //0 or 1
	int inside_temp; //0 to 100
	int crash; //0 or 1
	int lowlight; //0 or 1
}ecu_sensor;
typedef struct ecu_control{
	int ignition; //0 or 1 seatbelt, fuel_level
	int brake_status; //speed limit and obstacle 0 or 1
	int fan_status; //0 or 1
	int emergency_stop; //0 or 1 obstacle or collision/crash
	int airbag; //0 or 1, crash and obstacle high priority
	int ac_control; // 0 to 2 low mid high
	int fuel_status; //0 1 2 red yellow white
	int reverse_camera; //0 or 1
	int light_status; //0 or 1 gear 6 reverse camera and low light	
}ecu_control;

typedef struct{
    ecu_sensor sensor;
    ecu_control control;
    pthread_mutex_t lock;
}ECU;

int main(){
    printf("--- Subsystem Process ---\n");
    
    key_t key1 = 2345;
    
    sleep(2); // Wait for the sensor process to set up

    int shmid1 = shmget(key1, sizeof(ECU), 0666); 
    if(shmid1 == -1){
        perror("shmget failed to access segment. Is car.c running?");
        return 1;
    }     
    ECU* shm_ecu = (ECU*) shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU *)-1) {
        perror("shmat failed");
        return 1;
    }     
    
    printf("Waiting for car ignition...\n");
    
    // Wait until ignition is ON
    while(shm_ecu->control.ignition == 0) {
        sleep(1);
    }
    
    printf("Car Ignition is ON. Starting data read loop.\n");

    while(shm_ecu->control.ignition == 1){ 
        pthread_mutex_lock(&shm_ecu->lock);
        
        printf("\n[Subsystem] Reading data:\n");
        printf("Temp: %.2f || Speed: %.2f || Gear: %d || Fuel: %.2f\n", 
            shm_ecu->sensor.engine_temp, 
            shm_ecu->sensor.engine_speed, 
            shm_ecu->sensor.gear_pos, 
            shm_ecu->sensor.fuel_level);
        
        pthread_mutex_unlock(&shm_ecu->lock);       
        sleep(2);
    }   
     
    printf("[Subsystem] Ignition OFF. Exiting read loop.\n");
    

    shmdt(shm_ecu); 
    printf("--- Subsystem Process Exiting ---\n");
    return 0;
}
