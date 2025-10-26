#ifndef SENSOR_H
#define SENSOR_H 

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
}ecu_sensor;
typedef struct ecu_control{
	int ignition; //0 or 1 seatbelt, fuel_level
	int brake_status; //speed limit and obstacle 0 or 1
	int fan_status; //0 or 1
	int emergency_stop; //0 or 1 obstacle or collision/crash
	int airbag; //0 or 1, crash and obstacle high priority
	int ac_control; // 0 to 2 low mid high
	int fuel_status; //0 1 2 red yellow white
	int reverse_camera; //0 or 1, gear 6 reverse camera
	int back_light; //0 or 1, gear 6 back light	
}ecu_control;

typedef struct{
	ecu_sensor sensor;
	ecu_control control;
	pthread_mutex_t lock;
}ECU;

ECU* shm_ecu;

#endif
