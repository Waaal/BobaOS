#!/bin/bash
ip addr &
export DISPLAY=:1 &
Xvfb $DISPLAY -screen 0 1920x1080x16 &
xfce4-session &
sleep 5 &
x11vnc -forever -display $DISPLAY -loop -noxdamage -repeat -usepw -create &
/bin/bash
