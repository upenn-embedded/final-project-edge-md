#!/bin/bash
set -e  # Stop script immediately if any command fails

echo "Setting up Translation Display for Raspberry Pi..."

# Install system requirements for Tkinter and image processing (Pillow)
sudo apt-get update
sudo apt-get install -y python3-tk python3-venv libjpeg-dev zlib1g-dev libfreetype6-dev liblcms2-dev libopenjp2-7-dev libtiff5-dev

# Setup Virtual Environment
python3 -m venv venv

# Explicitly use the venv's pip to install requirements to ensure it goes to the right place
./venv/bin/pip install --upgrade pip
./venv/bin/pip install -r requirements.txt

echo "Setup complete! You can now run the app with ./start.sh"
