#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h> // wide char version of ncurses
#include "sensor.h"

void show_engine_status();
void draw_button(int y,int x,char* label,int color_pair);
void draw_control_button(int y, int x, const char* label, int color_pair);
void show_back_button();
void engine_on_screen();
void engine_off_screen();
void show_seatbelt_status();

ECU *shm_ecu;

void draw_simple_box(int y, int x, int h, int w, const char* title, int title_color_pair) {
    // Draw horizontal lines
    mvaddch(y, x, '+'); mvaddch(y, x + w, '+'); mvhline(y, x + 1, '-', w - 1);
    mvaddch(y + h, x, '+'); mvaddch(y + h, x + w, '+'); mvhline(y + h, x + 1, '-', w - 1);
    
    // Draw vertical lines
    for (int i = 1; i < h; i++) {
        mvaddch(y + i, x, '|');
        mvaddch(y + i, x + w, '|');
    }
    
    // Draw the Title
    attron(COLOR_PAIR(title_color_pair) | A_BOLD);
    mvprintw(y, x + 2, " %s ", title);
    attroff(COLOR_PAIR(title_color_pair) | A_BOLD);
}

// --- Emergency Stop Button ---
void draw_control_button(int y, int x, const char* label, int color_pair) {
    int w = 26;
    attron(COLOR_PAIR(color_pair) | A_BOLD );
    mvprintw(y, x, "+----------------------+");
    mvprintw(y + 1, x, "| %-20s |", label);
    mvprintw(y + 2, x, "+----------------------+");
    attroff(COLOR_PAIR(color_pair) | A_BOLD );
}

int main(){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    curs_set(0);
    start_color();
    setlocale(LC_ALL,""); 

    init_pair(1,COLOR_CYAN,COLOR_BLACK);    
    init_pair(2,COLOR_YELLOW,COLOR_BLACK);  
    init_pair(3,COLOR_GREEN,COLOR_BLACK);  
    init_pair(4,COLOR_WHITE,COLOR_BLUE);  
    init_pair(5,COLOR_WHITE,COLOR_BLACK);     
    init_pair(6, COLOR_RED, COLOR_BLACK);    // 🛑 Warnings
    init_pair(7, COLOR_GREEN, COLOR_BLACK);  // ✅ Normal
    init_pair(8, COLOR_MAGENTA, COLOR_BLACK); // ⚙️ System Controls

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
    mvaddwstr(2, 42, L"\U0001F697");
    mvprintw(2, 44, "REALTIME AUTOMATIVE CONTROL SYSTEM");
    mvaddwstr(2, 79, L"\U0001F697");    
    attroff(COLOR_PAIR(1)|A_BOLD);

    draw_button(5, 45,"1. Engine ON",2);
    draw_button(8,45,"2. Engine OFF",3);
    draw_button(11,45,"3. EXIT",4);

    mvprintw(14,45,"Select an option (1-3): ");
    refresh();

    choice=getch();

    if(choice=='1'){
        control.ignition=1;
        show_seatbelt_status();
    } 
    else if(choice=='2'){
        control.ignition=0;
        engine_off_screen();
    } 
    else if(choice=='3'){
        endwin();
        exit(0);
    } 
    else{
        attron(COLOR_PAIR(5));
        mvprintw(17, 45, "Invalid option! Press any key...");
        attroff(COLOR_PAIR(5));
        getch();
    }
}

void engine_off_screen(){
	clear();
    	attron(COLOR_PAIR(5)|A_BOLD);
    	mvprintw(5,45,"Engine is OFF. Please turn it ON to proceed.");
    	attroff(COLOR_PAIR(5));
    
    	mvprintw(7,45,"Press 'b' to go back to the Main Menu.");
    	refresh();

    	int ch=getch();
    	if(ch=='b'||ch=='B'){
        	return;
    	}
}

void show_seatbelt_status() {
    clear();
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddwstr(2, 48, L"\U0001F4B4");
    mvprintw(2, 50, "Seatbelt Status");
    mvaddwstr(2, 67, L"\U0001F4B4");
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    mvprintw(5, 45, "+----------------------------------+");
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(6, 45, "| *Is the seatbelt ON?* |");
    attroff(COLOR_PAIR(3) | A_BOLD);
    mvprintw(7, 45, "+----------------------------------+");
    
    mvprintw(9, 45, "+----------------------------------+");
    mvprintw(10, 45, "| Press '1' for Yes (Seatbelt ON)  |");
    mvprintw(11, 45, "+----------------------------------+");
    mvprintw(12, 45, "| Press '0' for No (Seatbelt OFF)  |");
    mvprintw(13, 45, "+----------------------------------+");
    refresh();

    int ch = getch();
    if (ch == '1') {
        engine_on_screen();
    } else if (ch == '0') {
        engine_off_screen();
    } else {
        mvprintw(15, 45, "Invalid input! Press any key...");
        getch();
        show_seatbelt_status();
        return;
    }
}

void engine_on_screen(){
    
    key_t key = 2345;
    
    int shm_id = shmget(key, sizeof(ECU), 0666);
    if (shm_id == -1) {
        endwin();
        perror("shmget failed (Is sensor.c running?)");
        exit(1);
    }

    shm_ecu = (ECU*) shmat(shm_id, NULL, 0);
    if (shm_ecu == (void*) -1) {
        endwin();
        perror("shmat failed");
        exit(1);
    }

    int ch;
    nodelay(stdscr, TRUE);

    const int COL1_X = 3;   // Left Column
    const int COL2_X = 45;  // Middle Column
    const int COL3_X = 87;  // Right Column
    const int BOX_W = 38;   // Box Width

    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);

        clear();
        attron(COLOR_PAIR(2)|A_BOLD);
        mvaddwstr(2, 38, L"\U0001F697");    
        mvprintw(2, 40,"ECU SENSOR DASHBOARD");
        mvaddwstr(2, 62, L"\U0001F697"); 
        attroff(COLOR_PAIR(2)|A_BOLD);
        
        //ENGINE TEMP & FAN
        draw_simple_box(4, COL1_X, 4, BOX_W, "Engine Temp & Fan", 8);        
        mvprintw(5, COL1_X + 2, "Temp: %.2f °C", shm_ecu->sensor.engine_temp); // Engine Temp            
        // Fan Status        
        mvprintw(6, COL1_X + 2, "Fan Status:");
        attron(COLOR_PAIR(7)|A_BOLD);
        mvprintw(6, COL1_X + 13," %s", (shm_ecu->control.fan_status) ? "ON (COOLING)" : "OFF");
        attroff(COLOR_PAIR(7)|A_BOLD);

	//SPEED & BRAKE (COL 2)
        draw_simple_box(4, COL2_X, 4, BOX_W, "Speed & Braking", 8);       
        mvprintw(5, COL2_X + 2, "Speed: %.2f RPM", shm_ecu->sensor.engine_speed); // Engine Speed      
        // Braking Status
        mvprintw(6, COL2_X + 2, "Brake Status:");
        attron(COLOR_PAIR(7)|A_BOLD);       
        //mvaddwstr(6, COL2_X + 15, (shm_ecu->sensor.engine_speed > 100) ? L"\u2193" : L"");
        mvprintw(6, COL2_X + 15," %s", shm_ecu->sensor.engine_speed > 100 ? "APPLIED" : "Normal");
        attroff(COLOR_PAIR(7)|A_BOLD);

        // 3. FUEL LEVEL & STATUS (COL 3)
        draw_simple_box(4, COL3_X, 4, BOX_W, "Fuel Level & Status", 8);                 
        mvprintw(5, COL3_X + 2, "Level: %.2f %%", shm_ecu->sensor.fuel_level); // Fuel Level       
        // Fuel Status
        mvprintw(6, COL3_X + 2, "Status:");
        if (shm_ecu->control.fuel_status == -1) {
            attron(COLOR_PAIR(6)|A_BOLD);
            mvaddwstr(6, COL3_X + 10, L"\u26A0"); // ⚠️
            printw(" LOW!");
            attroff(COLOR_PAIR(6)|A_BOLD);
        } else if (shm_ecu->control.fuel_status == 1) {
            attron(COLOR_PAIR(7)|A_BOLD);
            mvaddwstr(6, COL3_X + 10, L"\u26FD"); // ⛽
            printw(" FULL");
            attroff(COLOR_PAIR(7)|A_BOLD);
        } else {
            attron(COLOR_PAIR(7)|A_BOLD);
            mvaddwstr(6, COL3_X + 10, L"\u25CF"); // ⚫
            printw(" IDLE");
            attroff(COLOR_PAIR(7)|A_BOLD);
        }

        // 4. GEAR, LIGHTS & CAMERA (COL 1, below)
        draw_simple_box(9, COL1_X, 5, BOX_W, "Gear, Lights & Camera", 8);               
        mvprintw(10, COL1_X + 2, "Gear Position: %d", shm_ecu->sensor.gear_pos);       
        // Lights Status
        mvprintw(11, COL1_X + 2, "Back Lights:");
        mvaddwstr(11, COL1_X + 15, (shm_ecu->control.back_light) ? L"\u2600" : L""); // ☀
        printw(" %s", (shm_ecu->control.back_light) ? "ON" : "OFF");
        // Reverse Camera
        mvprintw(12, COL1_X + 2, "Rev. Camera:");
        mvaddwstr(12, COL1_X + 15, (shm_ecu->control.reverse_camera) ? L"\u21AA" : L""); // ↪ 
        printw(" %s", (shm_ecu->control.reverse_camera)?"ON":"OFF");


        // 5. OBSTACLE DETECTION & ACTION (COL 2, below)
        draw_simple_box(9, COL2_X, 5, BOX_W, "Obstacle Detection & Action", 8);        
        // Object Detected       
        mvprintw(10, COL2_X + 2, "Object Detected:");
        int obstacle_color = shm_ecu->sensor.obstacle_detector ? 6 : 7; 
        attron(COLOR_PAIR(obstacle_color)|A_BOLD);
        mvaddwstr(10, COL2_X + 19, (shm_ecu->sensor.obstacle_detector) ? L"\u26D4" : L""); // ⛔
        printw(" %s", (shm_ecu->sensor.obstacle_detector) ? "YES" : "NO");
        attroff(COLOR_PAIR(obstacle_color)|A_BOLD);     
        // Detection Action (Applied Brake/Decrease Speed)
        mvprintw(11, COL2_X + 2, "Action to take:");
        attroff(COLOR_PAIR(obstacle_color)|A_BOLD);
        mvaddwstr(11, COL2_X + 19, (shm_ecu->sensor.obstacle_detector) ? L"\u2193" : L""); // ↓ 
        printw(" %s", (shm_ecu->sensor.obstacle_detector) ? "DECREASE SPEED" : "Smooth");
        attroff(COLOR_PAIR(obstacle_color)|A_BOLD);


        // 6. CRASH, E-STOP & AIRBAG (COL 3, below)
        draw_simple_box(9, COL3_X, 5, BOX_W, "Crash, E-Stop & Airbag", 8);        
        // Crash Status       
        mvprintw(10, COL3_X + 2, "Crash Status:");
        int CRASH_COLOR = (shm_ecu->sensor.crash) ? 6 : 5;
        attron(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        mvaddwstr(10, COL3_X + 17, (shm_ecu->sensor.crash) ? L"\u2620" : L""); // ☠ or ✅
        printw(" %s", (shm_ecu->sensor.crash) ? "CRASH!" : "NORMAL");
        attroff(COLOR_PAIR(CRASH_COLOR)|A_BOLD);        
        // Emergency Stop Status
        mvprintw(11, COL3_X + 2, "E-Stop Status:");
        attron(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        mvaddwstr(11, COL3_X + 17, (shm_ecu->control.emergency_stop) ? L"\u26D4" : L""); // ⛔ or ✅
        printw(" %s", (shm_ecu->control.emergency_stop) ? "ACTIVE" : "NO");
        attroff(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        // Airbag Status
        mvprintw(12, COL3_X + 2, "Airbag Deploy:");
        attron(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        mvaddwstr(12, COL3_X + 17, (shm_ecu->control.airbag) ? L"\u26A0" : L""); // ⚠️ or ✅
        printw(" %s", (shm_ecu->control.airbag) ? "YES" : "NO");
        attroff(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        
        
        // EMERGENCY STOP BUTTON
        attron(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        draw_control_button(17, COL3_X + 6, "E: EMERGENCY STOP", 6);
        attroff(COLOR_PAIR(CRASH_COLOR)|A_BOLD);
        
        mvprintw(20, COL1_X, "Press 'b' to go back | Controls: [E] Emergency Stop");
        
        pthread_mutex_unlock(&shm_ecu->lock);

        refresh();

        ch = getch();
        if (ch == 'b' || ch == 'B') {
            nodelay(stdscr, FALSE);
            break;
        } else if (ch == 'e' || ch == 'E') {
             // Logic to simulate emergency stop command
            pthread_mutex_lock(&shm_ecu->lock);           
            shm_ecu->control.emergency_stop = 1; 
            sleep(1);
            engine_off_screen();           
            pthread_mutex_unlock(&shm_ecu->lock);
        }
	sleep(3);    
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
    // Instructions are now handled within engine_on_screen
}
