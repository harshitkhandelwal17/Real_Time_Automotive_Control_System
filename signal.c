#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

int main(){
pid_t car_pid;
printf("Enter Car PID: ");
scanf("%d",&car_pid);
int status;
printf("Press 1 to turn ON: ");
scanf("%d",&status);
if(status==1){
kill(car_pid,SIGUSR1);
}
printf("Press 0 to turn OFF: ");
scanf("%d",&status);
if(status==0){
kill(car_pid,SIGUSR2);
}
return 0;
}
