#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>

typedef struct ecu_sensor {
    float engine_temp;
    float engine_speed;
    int obstacle_detector;
    int gear_pos;
    float fuel_level;
    int seatbelt;
    int inside_temp;
    int crash;
    int lowlight;
} ecu_sensor;

typedef struct ecu_control {
    int ignition;
    int brake_status;
    int fan_status;
    int emergency_stop;
    int airbag;
    int ac_control;
    int fuel_status;
    int reverse_camera;
    int light_status;
} ecu_control;

typedef struct {
    ecu_sensor sensor;
    ecu_control control;
    pthread_mutex_t lock;
} ECU;

ECU* shm_ecu;

// === Controller Threads === //

void* fan_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        if (shm_ecu->sensor.engine_temp > 80.0 || shm_ecu->sensor.inside_temp > 35.0)
            shm_ecu->control.fan_status = 1;
        else
            shm_ecu->control.fan_status = 0;

        pthread_mutex_unlock(&shm_ecu->lock);
        usleep(500000);
    }
    return NULL;
}

void* brake_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        if (shm_ecu->sensor.obstacle_detector == 1 || shm_ecu->sensor.engine_speed > 120)
            shm_ecu->control.brake_status = 1;
        else
            shm_ecu->control.brake_status = 0;

        pthread_mutex_unlock(&shm_ecu->lock);
        usleep(500000);
    }
    return NULL;
}

void* light_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        shm_ecu->control.light_status = shm_ecu->sensor.lowlight ? 1 : 0;
        pthread_mutex_unlock(&shm_ecu->lock);
        usleep(500000);
    }
    return NULL;
}

void* safety_controller(void* arg) {
    while (1) {
        pthread_mutex_lock(&shm_ecu->lock);
        if (shm_ecu->control.ignition == 0) {
            pthread_mutex_unlock(&shm_ecu->lock);
            break;
        }

        if (shm_ecu->sensor.crash == 1) {
            shm_ecu->control.emergency_stop = 1;
            shm_ecu->control.airbag = 1;
        } else {
            shm_ecu->control.emergency_stop = 0;
            shm_ecu->control.airbag = 0;
        }

        pthread_mutex_unlock(&shm_ecu->lock);
        usleep(500000);
    }
    return NULL;
}

// === Main === //

int main() {
    printf("--- Subsystem Process ---\n");

    key_t key1 = 2345;
    int shmid1 = shmget(key1, sizeof(ECU), 0666);
    if (shmid1 == -1) {
        perror("shmget failed. Is sensor.c running?");
        return 1;
    }

    shm_ecu = (ECU*) shmat(shmid1, NULL, 0);
    if (shm_ecu == (ECU*) -1) {
        perror("shmat failed");
        return 1;
    }

    printf("Waiting for ignition ON...\n");
    while (shm_ecu->control.ignition == 0) sleep(1);

    printf("Ignition ON. Starting controllers...\n");

    pthread_t fan_thread, brake_thread, light_thread, safety_thread;
    pthread_create(&fan_thread, NULL, fan_controller, NULL);
    pthread_create(&brake_thread, NULL, brake_controller, NULL);
    pthread_create(&light_thread, NULL, light_controller, NULL);
    pthread_create(&safety_thread, NULL, safety_controller, NULL);

    pthread_join(fan_thread, NULL);
    pthread_join(brake_thread, NULL);
    pthread_join(light_thread, NULL);
    pthread_join(safety_thread, NULL);

    printf("Ignition OFF. Controllers stopped.\n");
    shmdt(shm_ecu);
    return 0;
}
