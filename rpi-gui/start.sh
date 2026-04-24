#!/bin/bash
# Export the default local display, helpful if running from SSH or autostart
export DISPLAY=:0

# Explicitly use the virtual environment's python executable
./venv/bin/python3 display.py
