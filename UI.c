#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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
    //ecu_sensor sensor={75.0,1200.0,1,3,50.0,1,22,0,0}; 
    key_t key=
    
    int shm_id=shmget(key,sizeof(ecu_sensor),0666|IPC_CREAT);
    if(shm_id==-1){
        perror("shmget");
        exit(1);
    }

    ecu_sensor* sensor=(ecu_sensor*)shmat(shm_id,NULL,0);
    if(sensor==(void*)-1){
        perror("shmat");
        exit(1);
    }
    
    int ch;
    nodelay(stdscr,TRUE);
    box(stdscr,0,0);
    clear();
    while(1){
        attron(COLOR_PAIR(2)|A_BOLD);
     	mvprintw(2,12,"ECU SENSOR DATA DASHBOARD");
     	attroff(COLOR_PAIR(2)|A_BOLD);
     	
        mvprintw(5,8,"+----------------------------------------+");
        mvprintw(6,8,"| %-35s %.2f °C    |", "1. Engine Temp:",sensor.engine_temp);
        mvprintw(7,8,"| %-35s %.2f RPM   |", "2. Engine Speed:",sensor.engine_speed);
        mvprintw(8,8,"| %-35s %.2f %%    |", "3. Fuel Level:",sensor.fuel_level);
        mvprintw(9,8,"| %-35s %d         |", "4. Gear Position:",sensor.gear_pos);
        mvprintw(10,8,"| %-35s %-3s      |", "5. Seatbelt:",(sensor.seatbelt==1)?"ON":"OFF");
        mvprintw(11,8,"| %-35s %-20s     |", "6. Crash Status:",(sensor.crash==1)?"CRASH DETECTED":"NO CRASH");
        mvprintw(12,8,"+----------------------------------------+");

        show_back_button();
        refresh();

        ch=getch();
        if(ch=='b'||ch=='B'){
            nodelay(stdscr,FALSE);
            break;
        }
    }
    shmdt(sensor);
}

void draw_button(int y,int x,char* label,int color_pair){
    attron(COLOR_PAIR(color_pair)|A_BOLD);
    mvprintw(y,x,"+----------------------------+");
    mvprintw(y+1,x,"| %-26s |",label);
    mvprintw(y+2,x,"+----------------------------+");
    attroff(COLOR_PAIR(color_pair)|A_BOLD);
}

void show_back_button(){
    mvprintw(14,8,"Press 'b' to go back to the Main Menu");
} 


