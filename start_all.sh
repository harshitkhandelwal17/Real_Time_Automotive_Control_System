#!/bin/bash

make all

echo "--- Starting ECU System (Rhythm Adjustment) ---"

gnome-terminal -- bash -c "echo 'Starting Sensor'; ./sensor; exec bash"
sleep 2 

gnome-terminal -- bash -c "echo 'Starting Server with sudo'; sudo ./server; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting Subsystem'; ./subsystem; exec bash"
sleep 1

gnome-terminal -- bash -c "echo 'Starting UI'; ./UI; exec bash"
sleep 1

echo "--- Running Signal Process to AUTO-START Ignition ---"
gnome-terminal -- bash -c "echo 'Starting Signal for Ignition Control'; ./signal; exec bash"
