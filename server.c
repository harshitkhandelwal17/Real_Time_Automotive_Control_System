#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/shm.h>
#include "sensor.h"

void send_html_page(int client_sock) {
    pthread_mutex_lock(&shm_ecu->lock);
    
    int is_crash = shm_ecu->sensor.crash;
    char crash_banner[512] = "";
    
    if (is_crash) {
        snprintf(crash_banner, sizeof(crash_banner),
            "<div style='background:#f00;color:#fff;padding:20px;font-size:24px;animation:blink 1s infinite;'>"
            "🚨 EMERGENCY ALERT: CRASH DETECTED 🚨<br>"
            "Location: Vehicle ID [%d]<br>"
            "Time: %ld<br>"
            "Status: Emergency services notified"
            "</div>", getpid(), time(NULL));
    }
    
    char response[8192];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta http-equiv='refresh' content='1'>"
        "<title>ECU Live Dashboard</title>"
        "<style>"
        "body{font-family:Arial;background:#111;color:#eee;text-align:center;}"
        "h2{color:#0f0;}"
        "table{margin:auto;border-collapse:collapse;}"
        "td,th{border:1px solid #555;padding:8px;}"
        "tr:nth-child(even){background:#222;}"
        ".crash{background:#f00;color:#fff;font-weight:bold;}"
        "@keyframes blink{0%%,100%%{opacity:1;}50%%{opacity:0.3;}}"
        "</style>"
        "</head><body>"
        "%s"  // Crash banner if crash detected
        "<h2>🚘 ECU Live Data Dashboard</h2>"
        "<table>"
        "<tr><th>Parameter</th><th>Value</th></tr>"
        "<tr><td>Ignition</td><td>%d</td></tr>"
        "<tr><td>Engine Temp (°C)</td><td>%.2f</td></tr>"
        "<tr><td>Engine Speed (RPM)</td><td>%.2f</td></tr>"
        "<tr><td>Gear Position</td><td>%d</td></tr>"
        "<tr><td>Fuel Level (%%)</td><td>%.2f</td></tr>"
        "<tr><td>Fan Status</td><td>%s</td></tr>"
        "<tr><td>Brake Status</td><td>%s</td></tr>"
        "<tr><td>Light Status</td><td>%s</td></tr>"
        "<tr class='%s'><td>Crash Status</td><td>%s</td></tr>"
        "<tr class='%s'><td>Airbag</td><td>%s</td></tr>"
        "<tr class='%s'><td>Emergency Stop</td><td>%s</td></tr>"
        "<tr class='%s'><td>Hazard Lights</td><td>%s</td></tr>"
        "<tr class='%s'><td>Horn/Alarm</td><td>%s</td></tr>"
        "<tr class='%s'><td>Doors</td><td>%s</td></tr>"
        "</table>"
        "<p style='margin-top:20px;color:#999;'>Page auto-refreshes every 1 second.</p>"
        "</body></html>",
        crash_banner,
        shm_ecu->control.ignition,
        shm_ecu->sensor.engine_temp,
        shm_ecu->sensor.engine_speed,
        shm_ecu->sensor.gear_pos,
        shm_ecu->sensor.fuel_level,
        shm_ecu->control.fan_status ? "ON" : "OFF",
        shm_ecu->control.brake_status ? "ON" : "OFF",
        shm_ecu->control.back_light ? "ON" : "OFF",
        is_crash ? "crash" : "",
        is_crash ? "CRASH DETECTED!" : "NORMAL",
        is_crash ? "crash" : "",
        shm_ecu->control.airbag ? "DEPLOYED" : "OK",
        is_crash ? "crash" : "",
        shm_ecu->control.emergency_stop ? "ACTIVE" : "OFF",
        shm_ecu->control.hazard_lights ? "crash" : "",
        shm_ecu->control.hazard_lights ? "BLINKING" : "OFF",
        shm_ecu->control.horn_alarm ? "crash" : "",
        shm_ecu->control.horn_alarm ? "ACTIVE" : "OFF",
        shm_ecu->control.doors_unlocked ? "crash" : "",
        shm_ecu->control.doors_unlocked ? "UNLOCKED" : "LOCKED"
    );
    pthread_mutex_unlock(&shm_ecu->lock);
    send(client_sock, response, strlen(response), 0);
}


void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char request[1024];
    recv(client_sock, request, sizeof(request) - 1, 0);

    // Always respond with the live dashboard
    send_html_page(client_sock);

    close(client_sock);
    return NULL;
}

int main() {
    key_t key = 9876;
    int shmid = shmget(key, sizeof(ECU), 0666);
    if (shmid == -1) {
        perror("shmget failed (Is sensor.c running?)");
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

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 5);
    printf("HTTP Server running on http://localhost\n");

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
