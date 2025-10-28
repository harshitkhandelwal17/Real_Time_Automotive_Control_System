#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "sensor.h"

int log_fd = -1;

void write_log(const char* message) {
    if (log_fd == -1) return;
    
    time_t now;
    struct tm* timeinfo;
    time(&now);
    char timestamp[64];
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "[ %Y-%m-%d %H:%M:%S ]", timeinfo);
    
    write(log_fd, timestamp, strlen(timestamp));
    write(log_fd, message, strlen(message));
    write(log_fd, "\n", 1);
}

// === Controller Threads === //
void* fan_controller(void* arg) {
    write_log("Fan controller thread started");
    
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        int prev_status = shm_ecu->control.fan_status;
        if (shm_ecu->sensor.engine_temp > 80.0)
            shm_ecu->control.fan_status = 1;
        else
            shm_ecu->control.fan_status = 0;

        if (prev_status != shm_ecu->control.fan_status) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Fan %s (Engine temp: %.2f°C)",
                     shm_ecu->control.fan_status ? "ON" : "OFF",
                     shm_ecu->sensor.engine_temp);
            write_log(log_msg);
        }

        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Fan controller thread exiting");
    return NULL;
}

void* ac_controller(void* arg){
    write_log("AC controller thread started");
    
    while(1){
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        int prev_status = shm_ecu->control.ac_control;
        if (shm_ecu->sensor.inside_temp > 24.0)
            shm_ecu->control.ac_control = 1;
        else
            shm_ecu->control.ac_control = 0;

        if (prev_status != shm_ecu->control.ac_control) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "AC %s (Inside temp: %d°C)",
                     shm_ecu->control.ac_control ? "ON" : "OFF",
                     shm_ecu->sensor.inside_temp);
            write_log(log_msg);
        }

        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("AC controller thread exiting");
    return NULL;
}

void* brake_controller(void* arg) {
    write_log("Brake controller thread started");
    
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        int prev_status = shm_ecu->control.brake_status;
        if (shm_ecu->sensor.obstacle_detector == 1 || shm_ecu->sensor.engine_speed > 100)
            shm_ecu->control.brake_status = 1;	
        else
            shm_ecu->control.brake_status = 0;

        if (prev_status != shm_ecu->control.brake_status) {
            if (shm_ecu->control.brake_status) {
                if (shm_ecu->sensor.obstacle_detector == 1)
                    write_log("Brakes APPLIED - Obstacle detected");
                else
                    write_log("Brakes APPLIED - Speed exceeded 100 RPM");
            } else {
                write_log("Brakes RELEASED");
            }
        }

        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Brake controller thread exiting");
    return NULL;
}

void* light_controller(void* arg) {
    write_log("Light controller thread started");
    
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        int prev_back = shm_ecu->control.back_light;
        shm_ecu->control.back_light = (shm_ecu->sensor.gear_pos == 6) ? 1 : 0;  
        shm_ecu->control.reverse_camera = (shm_ecu->sensor.gear_pos == 6) ? 1 : 0;
        
        if (prev_back != shm_ecu->control.back_light) {
            if (shm_ecu->control.back_light) {
                write_log("Reverse lights ON - Gear in reverse (6)");
            } else {
                write_log("Reverse lights OFF");
            }
        }
        
        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Light controller thread exiting");
    return NULL;
}

void* safety_controller(void* arg) {
    write_log("Safety controller thread started");
    
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        if (shm_ecu->sensor.crash == 1) {
            shm_ecu->control.emergency_stop = 1;
            shm_ecu->control.airbag = 1;
            shm_ecu->control.hazard_lights = 1;
            shm_ecu->control.horn_alarm = 1;
            shm_ecu->control.doors_unlocked = 1;
            shm_ecu->control.ignition = 0;
            
            printf("[CRASH SAFETY] All emergency systems activated!\n");
            write_log("CRITICAL: Crash detected - all emergency systems activated");
            write_log("Emergency systems: Airbag deployed, Emergency stop, Hazard lights, Horn alarm, Doors unlocked");
        } else {
            shm_ecu->control.emergency_stop = 0;
            shm_ecu->control.airbag = 0;
            shm_ecu->control.hazard_lights = 0;
            shm_ecu->control.horn_alarm = 0;
            shm_ecu->control.doors_unlocked = 0;
        }
        
        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Safety controller thread exiting");
    return NULL;
}

void* fuel_controller(void* arg){
    write_log("Fuel controller thread started");
    
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }
        
        int prev_status = shm_ecu->control.fuel_status;
        if (shm_ecu->sensor.fuel_level < 25.0f) {
            shm_ecu->control.fuel_status = 0;
        } else {
            shm_ecu->control.fuel_status = 1;
        }
        
        if (prev_status != shm_ecu->control.fuel_status) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Fuel status: %s (Level: %.1f%%)",
                     shm_ecu->control.fuel_status ? "NORMAL" : "LOW",
                     shm_ecu->sensor.fuel_level);
            write_log(log_msg);
        }
        
        pthread_mutex_unlock(&shm_ecu->lock);
        sleep(3);
    }
    
    write_log("Fuel controller thread exiting");
    return NULL;
}

int main() {
    printf("--- Subsystem Process ---\n");
    
    // Open log file
    log_fd = open("subsystem.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Failed to open log file");
    } else {
        write_log("========== Subsystem Process Started ==========");
    }

    key_t key1 = 9876;
    int shmid1 = shmget(key1, sizeof(ECU), 0666);
    if (shmid1 == -1) {
        perror("shmget failed. Is sensor.c running?");
        write_log("ERROR: Failed to get shared memory. Is sensor.c running?");
        return 1;
    }

    shm_ecu = (ECU*) shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        write_log("ERROR: Failed to attach shared memory");
        return 1;
    }
    
    write_log("Shared memory attached successfully");

    printf("Waiting for ignition ON...\n");
    write_log("Waiting for ignition ON signal...");
    
    while (shm_ecu->control.ignition == 0) sleep(1);

    printf("Ignition ON. Starting controllers...\n");
    write_log("Ignition ON - Starting all controller threads");

    pthread_t fan_thread, ac_thread, brake_thread, light_thread, safety_thread, fuel_thread;
    pthread_create(&fan_thread, NULL, fan_controller, NULL);
    pthread_create(&ac_thread, NULL, ac_controller, NULL);
    pthread_create(&brake_thread, NULL, brake_controller, NULL);
    pthread_create(&light_thread, NULL, light_controller, NULL);
    pthread_create(&safety_thread, NULL, safety_controller, NULL);
    pthread_create(&fuel_thread, NULL, fuel_controller, NULL);

    pthread_join(fan_thread, NULL);
    pthread_join(ac_thread, NULL);
    pthread_join(brake_thread, NULL);
    pthread_join(light_thread, NULL);
    pthread_join(safety_thread, NULL);
    pthread_join(fuel_thread, NULL);

    printf("Ignition OFF. Controllers stopped.\n");
    write_log("Ignition OFF - All controllers stopped");
    write_log("========== Subsystem Process Exiting ==========");
    
    shmdt(shm_ecu);
    
    if (log_fd != -1) {
        close(log_fd);
    }
    
    return 0;
}
