#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>
#include<signal.h>

typedef struct ecu_sensor{
	float engine_temp; //random 80 - 100 degree avg
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

volatile int car_status = 1; //ON

ecu_sensor sensor = {0.0,0.0,0,0,100.0,0,0,0,0};
ecu_control control = {0,0,0,0,0,0,0,0,0};

void* engine_handler(void* arg)
{
while(car_status){
	sensor.engine_temp = 80+ rand() % 21; //80 - 100
	sensor.engine_speed = 20 + rand() % 81; // 20 - 100
	sensor.gear_pos = 1 + rand() % 6; // 1-6
	sleep(1);
	printf("Temp: %f Speed: %f Gear: %d Fuel: %f\n", sensor.engine_temp, sensor.engine_speed, sensor.gear_pos, 	sensor.fuel_level);
}
pthread_exit(NULL);
}

void car_status_handler(int sig){
if(sig==SIGUSR1){
car_status = 0;
}
}

int main()
{
printf("Process ID: %d\n",getpid());
srand(time(NULL));
signal(SIGUSR1,car_status_handler);

pthread_t engine_temp, engine_speed, obstacle_detector,gear_pos, fuel_level, seatbelt, inside_temp, crash, lowlight;
pthread_create(&engine_temp, NULL, engine_handler, NULL);
pthread_create(&engine_speed, NULL, engine_handler, NULL);
pthread_create(&gear_pos, NULL, engine_handler, NULL);
pthread_create(&fuel_level, NULL, engine_handler, NULL); 


while(car_status){
sleep(1);
}

pthread_join(engine_temp, NULL);
pthread_join(engine_speed, NULL);
pthread_join(gear_pos, NULL);
pthread_join(fuel_level, NULL);


printf("Hello this is sensor file\n");
return 0;
}
