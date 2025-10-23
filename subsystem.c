#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>

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

int main(){
    key_t key1 = 2345;
    key_t key2 = 5678;

    printf("Inside subsystem process\n");

    int shmid1 = shmget(key1, 1024, 0666); 
    if(shmid1 == -1){
        perror("shmget failed to access segment");
        return 1;
    }    
    ecu_sensor* shm1 = (ecu_sensor*) shmat(shmid1, NULL, 0);
    if (shm1 == (ecu_sensor *)-1) {
        perror("shmat failed");
        return 1;
    }
    
    int shmid2 = shmget(key2, 1024, 0666); 
    if(shmid1 == -1){
        perror("shmget failed to access segment");
        return 1;
    }   
    ecu_control* shm2 = (ecu_control*) shmat(shmid2, NULL, 0);
    if (shm2 == (ecu_control *)-1) {
        perror("shmat failed");
        return 1;
    }
    
    while(shm2->ignition){
	    printf("\nReading shared memory from subsystem:\n");
	    printf("Engine temp: %.2f\n", shm1->engine_temp);
	    printf("Engine speed: %.2f\n", shm1->engine_speed);
	    printf("obstacle_detector: %d\n", shm1->obstacle_detector);
	    printf("gear_pos: %d\n", shm1->gear_pos);
	    printf("fuel_level: %.2f\n", shm1->fuel_level);
	    sleep(2);
     }   
	  
    shmdt(shm1);
    shmdt(shm2);
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    
    return 0;
}
