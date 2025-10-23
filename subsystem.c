#include<stdio.h>
#include<stdlib.h>

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

int main()
{
printf("Hello this is subsystem file\n");
return 0;
}
