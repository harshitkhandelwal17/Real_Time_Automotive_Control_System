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

ECU* shm_ecu;
pthread_t engine_thread;
int thread_created = 0;

void* engine_handler(void* arg)
{
	while(shm_ecu->control.ignition){ 
		pthread_mutex_lock(&shm_ecu->lock);
		shm_ecu->sensor.engine_temp = 80 + rand() % 21;
		shm_ecu->sensor.engine_speed = 20 + rand() % 81;
		shm_ecu->sensor.gear_pos = 1 + rand() % 6; 
		shm_ecu->sensor.fuel_level = 100.0f - (float)(rand() % 5);
        
		printf("[Engine Thread] Temp: %.2f || Speed: %.2f || Gear: %d || Fuel: %.2f\n", 
			shm_ecu->sensor.engine_temp, 
			shm_ecu->sensor.engine_speed, 
			shm_ecu->sensor.gear_pos, 
			shm_ecu->sensor.fuel_level);
        
		pthread_mutex_unlock(&shm_ecu->lock);
		sleep(2);
	}
        printf("[Engine Thread] Ignition turned off. Exiting thread.\n");
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
            printf("[Signal Handler] Engine thread started.\n");
        } else {
            perror("[Signal Handler] Thread creation failed");
        }
    }
}

int main()
{
    printf("--- Car Main Process ---\n");
    printf("Process ID: %d\n",getpid());	
    srand(time(NULL));
    key_t key1 = 2345;	

    int shmid1 = shmget(key1, sizeof(ECU), 0666 | IPC_CREAT);
    if (shmid1 == -1) {
        perror("shmget failed");
        exit(1);
    }
    printf("Shared Memory ID: %d\n", shmid1);
    shm_ecu = (ECU *)shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    memset(shm_ecu, 0, sizeof(ECU));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); 
    
    if (pthread_mutex_init(&shm_ecu->lock, &attr) != 0) {
        perror("Shared Mutex initialization failed");
        exit(1);
    }
    pthread_mutexattr_destroy(&attr);
    
    signal(SIGUSR1, car_status_handler);
    signal(SIGUSR2, car_status_handler);

    printf("Waiting for Ignition ON signal (SIGUSR1)...\n");
    
    while(1) {
        if (thread_created == 1 && shm_ecu->control.ignition == 0) {                 
            if (pthread_join(engine_thread, NULL) == 0) {               
                break;
            } else {               
                break;
            }
        }
        sleep(1);
    }
    
    shmdt(shm_ecu);       
    shmctl(shmid1, IPC_RMID, NULL);         
    printf("--- Car Main Process Exiting ---\n");

    return 0;
}
