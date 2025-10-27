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
    int ignition;
    int brake_status;
    int fan_status;
    int emergency_stop;
    int airbag;
    int ac_control;
    int fuel_status;
    int reverse_camera;
    int back_light;
    int hazard_lights;    // NEW: 0=OFF, 1=ON (blink after crash)
    int horn_alarm;       // NEW: 0=OFF, 1=ON (active after crash)
    int doors_unlocked;   // NEW: 0=LOCKED, 1=UNLOCKED
    int crash_alert_sent; // NEW: 0=NOT SENT, 1=SENT to server
}ecu_control;

typedef struct{
	ecu_sensor sensor;
	ecu_control control;
	pthread_mutex_t lock;
    pid_t pid;
}ECU;

ECU* shm_ecu;

#endif
