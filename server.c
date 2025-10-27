#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/shm.h>
#include <time.h>
#include "sensor.h"

void send_html_page(int client_sock) {
    pthread_mutex_lock(&shm_ecu->lock);
    
    int is_crash = shm_ecu->sensor.crash;
    int is_ignition_on = shm_ecu->control.ignition;
    
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
    
    char response[12288];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta http-equiv='refresh' content='2'>"
        "<title>ECU Dashboard</title>"
        "<style>"
        "body { font-family: Arial; background: #1a1a2e; color: #eee; margin: 0; padding: 20px; }"
        ".header { text-align: center; background: #16213e; padding: 20px; border-radius: 10px; margin-bottom: 20px; }"
        ".header h1 { margin: 0; color: #0f4c81; }"
        ".status { display: inline-block; padding: 5px 15px; border-radius: 20px; margin: 10px; font-weight: bold; }"
        ".status-on { background: #00ff88; color: #000; }"
        ".status-off { background: #ff4757; color: #fff; }"
        ".crash-alert { background: #ff4757; padding: 20px; border-radius: 10px; margin-bottom: 20px; text-align: center; }"
        ".crash-alert h2 { margin: 0 0 10px 0; font-size: 24px; }"
        ".engine-off { text-align: center; padding: 60px 20px; background: #16213e; border-radius: 10px; }"
        ".container { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 15px; }"
        ".card { background: #16213e; padding: 20px; border-radius: 10px; }"
        ".card h3 { margin: 0 0 15px 0; color: #4a9eff; font-size: 18px; border-bottom: 2px solid #0f4c81; padding-bottom: 10px; }"
        ".row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #0f4c81; }"
        ".row:last-child { border-bottom: none; }"
        ".label { color: #aaa; }"
        ".value { font-weight: bold; font-size: 18px; }"
        ".badge { padding: 4px 10px; border-radius: 15px; font-size: 13px; font-weight: bold; }"
        ".badge-green { background: #00ff88; color: #000; }"
        ".badge-red { background: #ff4757; color: #fff; }"
        ".badge-yellow { background: #ffd32a; color: #000; }"
        ".badge-blue { background: #4a9eff; color: #000; }"
        ".footer { text-align: center; margin-top: 20px; color: #aaa; font-size: 14px; }"
        "</style>"
        "</head><body>"
        
        "<div class='header'>"
        "<h1>ECU Dashboard</h1>"
        "<span class='status %s'>Engine: %s</span>"
        "<div style='color: #aaa; margin-top: 10px;'>PID: %d | Time: %s</div>"
        "</div>",
        is_ignition_on ? "status-on" : "status-off",
        is_ignition_on ? "ON" : "OFF",
        getpid(), time_str
    );
    
    send(client_sock, response, strlen(response), 0);
    
    // Crash Alert
    if (is_crash) {
        char crash_html[1024];
        snprintf(crash_html, sizeof(crash_html),
            "<div class='crash-alert'>"
            "<h2>CRASH DETECTED</h2>"
            "<p>All emergency systems activated!</p>"
            "<p>Vehicle ID: %d | Emergency services notified</p>"
            "</div>",
            getpid()
        );
        send(client_sock, crash_html, strlen(crash_html), 0);
    }
    
    // Engine OFF
    if (!is_ignition_on && !is_crash) {
        char off_html[512];
        snprintf(off_html, sizeof(off_html),
            "<div class='engine-off'>"
            "<h2>Engine is OFF</h2>"
            "<p>Start the ignition to view sensor data</p>"
            "</div>"
            "<div class='footer'>Auto-refresh: 2 seconds</div>"
            "</body></html>"
        );
        send(client_sock, off_html, strlen(off_html), 0);
        pthread_mutex_unlock(&shm_ecu->lock);
        return;
    }
    
    // Main Dashboard
    char cards[8192];
    snprintf(cards, sizeof(cards),
        "<div class='container'>"
        
        // Card 1: Engine
        "<div class='card'>"
        "<h3>Engine</h3>"
        "<div class='row'>"
        "<span class='label'>Temperature</span>"
        "<span class='value'>%.1f °C</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Speed</span>"
        "<span class='value'>%.0f RPM</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Fan</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "</div>"
        
        // Card 2: Fuel
        "<div class='card'>"
        "<h3>Fuel</h3>"
        "<div class='row'>"
        "<span class='label'>Level</span>"
        "<span class='value'>%.1f %%</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Status</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "</div>"
        
        // Card 3: Braking
        "<div class='card'>"
        "<h3>Braking</h3>"
        "<div class='row'>"
        "<span class='label'>Brake Status</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Obstacle</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "</div>"
        
        // Card 4: Transmission
        "<div class='card'>"
        "<h3>Transmission</h3>"
        "<div class='row'>"
        "<span class='label'>Gear Position</span>"
        "<span class='value'>%d</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Reverse Light</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Camera</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "</div>"
        
        // Card 5: Safety
        "<div class='card'>"
        "<h3>Safety</h3>"
        "<div class='row'>"
        "<span class='label'>Crash</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>Airbag</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "<div class='row'>"
        "<span class='label'>E-Stop</span>"
        "<span class='badge %s'>%s</span>"
        "</div>"
        "</div>",
        
        // Engine Card Data
        shm_ecu->sensor.engine_temp,
        shm_ecu->sensor.engine_speed,
        shm_ecu->control.fan_status ? "badge-green" : "badge-blue",
        shm_ecu->control.fan_status ? "ON" : "OFF",
        
        // Fuel Card Data
        shm_ecu->sensor.fuel_level,
        shm_ecu->control.fuel_status == 1 ? "badge-green" : (shm_ecu->control.fuel_status == -1 ? "badge-red" : "badge-blue"),
        shm_ecu->control.fuel_status == 1 ? "FULL" : (shm_ecu->control.fuel_status == -1 ? "LOW" : "NORMAL"),
        
        // Braking Card Data
        shm_ecu->control.brake_status ? "badge-red" : "badge-green",
        shm_ecu->control.brake_status ? "APPLIED" : "OFF",
        shm_ecu->sensor.obstacle_detector ? "badge-red" : "badge-green",
        shm_ecu->sensor.obstacle_detector ? "YES" : "NO",
        
        // Transmission Card Data
        shm_ecu->sensor.gear_pos,
        shm_ecu->control.back_light ? "badge-yellow" : "badge-blue",
        shm_ecu->control.back_light ? "ON" : "OFF",
        shm_ecu->control.reverse_camera ? "badge-green" : "badge-blue",
        shm_ecu->control.reverse_camera ? "ON" : "OFF",
        
        // Safety Card Data
        is_crash ? "badge-red" : "badge-green",
        is_crash ? "CRASH!" : "NORMAL",
        shm_ecu->control.airbag ? "badge-red" : "badge-green",
        shm_ecu->control.airbag ? "DEPLOYED" : "OK",
        shm_ecu->control.emergency_stop ? "badge-red" : "badge-blue",
        shm_ecu->control.emergency_stop ? "ACTIVE" : "OFF"
    );
    send(client_sock, cards, strlen(cards), 0);
    
    // Emergency Card (only if crash)
    if (is_crash) {
        char emergency[1024];
        snprintf(emergency, sizeof(emergency),
            "<div class='card' style='border: 2px solid #ff4757;'>"
            "<h3 style='color: #ff4757;'>Emergency</h3>"
            "<div class='row'>"
            "<span class='label'>Hazard Lights</span>"
            "<span class='badge %s'>%s</span>"
            "</div>"
            "<div class='row'>"
            "<span class='label'>Horn</span>"
            "<span class='badge %s'>%s</span>"
            "</div>"
            "<div class='row'>"
            "<span class='label'>Doors</span>"
            "<span class='badge %s'>%s</span>"
            "</div>"
            "</div>",
            shm_ecu->control.hazard_lights ? "badge-yellow" : "badge-blue",
            shm_ecu->control.hazard_lights ? "BLINKING" : "OFF",
            shm_ecu->control.horn_alarm ? "badge-red" : "badge-blue",
            shm_ecu->control.horn_alarm ? "ACTIVE" : "OFF",
            shm_ecu->control.doors_unlocked ? "badge-yellow" : "badge-green",
            shm_ecu->control.doors_unlocked ? "UNLOCKED" : "LOCKED"
        );
        send(client_sock, emergency, strlen(emergency), 0);
    }
    
    char footer[256];
    snprintf(footer, sizeof(footer),
        "</div>"
        "<div class='footer'>Auto-refresh: 2 seconds | Last update: %s</div>"
        "</body></html>",
        time_str
    );
    send(client_sock, footer, strlen(footer), 0);
    
    pthread_mutex_unlock(&shm_ecu->lock);
}

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char request[1024];
    recv(client_sock, request, sizeof(request), 0);
    send_html_page(client_sock);
    close(client_sock);
    
    return NULL;
}

int main() {
    printf("=== ECU Web Server ===\n");
    
    key_t key = 9876;
    int shmid = shmget(key, sizeof(ECU), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    shm_ecu = (ECU*) shmat(shmid, NULL, 0);
    if (shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        exit(1);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 10);
    printf("Server running at: http://localhost:8080\n");

    while (1) {
        int client_sock = accept(server_fd, NULL, NULL);
        if (client_sock < 0) continue;

        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        
        pthread_t t;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }

    close(server_fd);
    shmdt(shm_ecu);
    return 0;
}
