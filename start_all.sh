#!/bin/bash

# Open terminal windows and run each program
gnome-terminal -- bash -c "echo 'Starting Sensor'; ./sensor; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting Signal'; ./signal; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting Subsystem'; ./subsystem; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting UI'; ./UI; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting Server with sudo'; sudo ./server; exec bash"

