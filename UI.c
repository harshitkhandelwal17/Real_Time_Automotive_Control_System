#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

typedef struct ecu_sensor{
    float engine_temp;  // random
    float engine_speed; // random
    int obstacle_detector; // 0/1
    int gear_pos; // 1-6
    float fuel_level; // 0 to 100
    int seatbelt; // 0 or 1
    int inside_temp; // 0 to 100
    int crash; // 0 or 1
    int lowlight; // 0 or 1
}ecu_sensor;

typedef struct ecu_control{
    int ignition; // 0 or 1
    int brake_status; // 0 or 1
    int fan_status; // 0 or 1
    int emergency_stop; // 0 or 1
    int airbag; // 0 or 1
    int ac_control; // 0 to 2 (low, mid, high)
    int fuel_status; // 0 1 2 (red, yellow, white)
    int reverse_camera; // 0 or 1
    int light_status; // 0 or 1
}ecu_control;

typedef struct {
    ecu_sensor sensor;
    ecu_control control;
    pthread_mutex_t lock;
} ECU;

void show_engine_status();
void show_sensor_data();
void draw_button(int y,int x,char* label,int color_pair);
void show_back_button();
void engine_on_screen();
void engine_off_screen();

int main(){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    curs_set(0);
    start_color();

    init_pair(1,COLOR_CYAN,COLOR_BLACK);    
    init_pair(2,COLOR_YELLOW,COLOR_BLACK);  
    init_pair(3,COLOR_GREEN,COLOR_BLACK);  
    init_pair(4,COLOR_WHITE,COLOR_BLUE);  
    init_pair(5,COLOR_BLACK,COLOR_YELLOW); 

    srand(time(0));
    while(1){
        show_engine_status();
    }
    endwin();
    return 0;
}

void show_engine_status(){
    int choice=0;
    ecu_control control={0,0,0,0,0,0,0,0,0};
    
    clear();
    attron(COLOR_PAIR(1)|A_BOLD);
    mvprintw(1,18,"=== REALTIME AUTOMATIVE CONTROL SYSTEM===");
    attroff(COLOR_PAIR(1)|A_BOLD);

    draw_button(4,10,"1. Engine ON",2);
    draw_button(7,10,"2. Engine OFF",3);
    draw_button(10,10,"3. EXIT",4);

    mvprintw(13,10,"Select an option (1-3): ");
    refresh();

    choice=getch();

    if(choice=='1'){
        control.ignition=1; //Engine ON
        engine_on_screen();
    } 
    else if(choice=='2'){
        control.ignition=0; //Engine OFF
        engine_off_screen();
    } 
    else if(choice=='3'){
        endwin();
        exit(0);
    } 
    else{
        attron(COLOR_PAIR(5));
        mvprintw(15, 10, "Invalid option! Press any key...");
        attroff(COLOR_PAIR(5));
        getch();
    }
}

void engine_off_screen(){
	clear();
    	attron(COLOR_PAIR(5)|A_BOLD);
    	mvprintw(5,10,"Engine is OFF.Please turn it ON to proceed.");
    	attroff(COLOR_PAIR(5));
    
    	mvprintw(7,10,"Press 'b' to go back to the Main Menu.");
    	refresh();

    	int ch=getch();
    	if(ch=='b'||ch=='B'){
        	show_engine_status();
    	}
}

void engine_on_screen(){
    
    key_t key = 2345;
    
    int shm_id = shmget(key, sizeof(ECU), 0666);
    if (shm_id == -1) {
        endwin();
        perror("shmget failed (Is car/sensor.c running?)");
        exit(1);
    }

     ECU* shm_ecu = (ECU*) shmat(shm_id, NULL, 0);
    if (shm_ecu == (void*) -1) {
        endwin();
        perror("shmat failed");
        exit(1);
    }
    int ch;
    nodelay(stdscr, TRUE);
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);

        clear();
        attron(COLOR_PAIR(2)|A_BOLD);
        mvprintw(2,12,"=== ECU SENSOR DATA DASHBOARD ===");
        attroff(COLOR_PAIR(2)|A_BOLD);

        mvprintw(5,8,"Engine Temp      : %.2f °C", shm_ecu->sensor.engine_temp);
        mvprintw(6,8,"Engine Speed     : %.2f RPM", shm_ecu->sensor.engine_speed);
        mvprintw(7,8,"Gear Position    : %d", shm_ecu->sensor.gear_pos);
        mvprintw(8,8,"Reverse Camera Status    : %s", (shm_ecu->sensor.gear_pos==6)?"ON":"OFF");
        mvprintw(9,8,"Fuel Level       : %.2f %%", shm_ecu->sensor.fuel_level);
        mvprintw(10,8,"Seatbelt         : %s", (shm_ecu->sensor.seatbelt) ? "ON" : "OFF");
        mvprintw(11,8,"Crash Status     : %s", (shm_ecu->sensor.crash) ? "CRASH DETECTED" : "NORMAL");
        mvprintw(12,8,"Low Light        : %s", (shm_ecu->sensor.lowlight) ? "YES" : "NO");
        mvprintw(13,8,"Fan Status        : %s", (shm_ecu->control.fan_status) ? "YES" : "NO");
         mvprintw(14,8,"Brake Status        : %s", (shm_ecu->control.brake_status) ? "YES" : "NO");
         mvprintw(15,8,"Light Status        : %s", (shm_ecu->control.light_status) ? "YES" : "NO");
         mvprintw(16,8,"Emergency Stop       : %s", (shm_ecu->control.emergency_stop) ? "YES" : "NO");
         mvprintw(17,8,"Airbag        : %s", (shm_ecu->control.airbag) ? "YES" : "NO");



        pthread_mutex_unlock(&shm_ecu->lock);

        show_back_button();
        refresh();

        ch = getch();
        if (ch == 'b' || ch == 'B') {
            nodelay(stdscr, FALSE);
            break;
        }

        usleep(500000); // Refresh every 0.5s
    }

    shmdt(shm_ecu);
}
     

void draw_button(int y,int x,char* label,int color_pair){
    attron(COLOR_PAIR(color_pair)|A_BOLD);
    mvprintw(y,x,"+----------------------------+");
    mvprintw(y+1,x,"| %-26s |",label);
    mvprintw(y+2,x,"+----------------------------+");
    attroff(COLOR_PAIR(color_pair)|A_BOLD);
}

void show_back_button(){
    mvprintw(19,8,"Press 'b' to go back to the Main Menu");
} 

