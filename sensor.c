#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
pthread_mutex_t lock;

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

ecu_sensor sensor;
ecu_control control;
ecu_sensor* shm1;

void* engine_handler(void* arg)
{
	while(control.ignition){
		pthread_mutex_lock(&lock);
		sensor.engine_temp = 80+ rand() % 21; //80 - 100
		sensor.engine_speed = 20 + rand() % 81; // 20 - 100
		sensor.gear_pos = 1 + rand() % 6; // 1-6		
		printf("Temp: %f Speed: %f Gear: %d Fuel: %f\n", sensor.engine_temp, sensor.engine_speed, sensor.gear_pos, 	sensor.fuel_level);
		memcpy(shm1, &sensor, sizeof(ecu_sensor));
		pthread_mutex_unlock(&lock);
		sleep(2);
	}
	pthread_exit(NULL);
}

void car_status_handler(int sig){
	if(sig==SIGUSR1){
		control.ignition = 0;
	}
}

int main()
{
	control.ignition = 1;
	srand(time(NULL));		
 	
    	signal(SIGUSR1,car_status_handler);
    	
    	pthread_mutex_init(&lock,NULL);
 	printf("Process ID: %d\n",getpid());
 	
 	key_t key1 = 2345;
 	key_t key2 = 5678; 	
 	
    	int shmid1 = shmget(key1, 1024, 0666 | IPC_CREAT);
    	if (shmid1 == -1) {
        	perror("shmget failed");
        	exit(1);
    	}
    	printf("shmid of shared memory is %d\n", shmid1);    	
    	shm1 = shmat(shmid1, NULL, 0);
    	if (shm1 == (ecu_sensor *)-1) {
        	perror("shmat failed");
        	exit(1);
    	}
    		
    	int shmid2 = shmget(key2, 1024, 0666 | IPC_CREAT);
    	if (shmid2 == -1) {
        	perror("shmget failed");
        	exit(1);
    	}
    	printf("shmid of shared memory is %d\n", shmid1);    	
    	ecu_control* shm2 = shmat(shmid2, NULL, 0);
    	if (shm2 == (ecu_control *)-1) {
        	perror("shmat failed");
        	exit(1);
    	}
    	memcpy(shm2, &control, sizeof(ecu_control));
    	 	
	pthread_t engine_temp;
	
	pthread_create(&engine_temp, NULL, engine_handler, NULL);

	pthread_join(engine_temp, NULL);
	
return 0;
}
