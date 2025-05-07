# Use Ubuntu 24.04 as the base image
FROM ubuntu:24.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install sudo and other prerequisites
RUN apt-get update && apt-get install -y \
    sudo \
    curl \
    gnupg2 \
    lsb-release \
    locales \
    dos2unix \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# Copy your ROS 2 installer script into the container
COPY your_ros2_installer.sh /tmp/

# Convert the script to Unix line endings
# (SOME BULLSHIT)
RUN dos2unix /tmp/your_ros2_installer.sh

# Make the installer script executable
RUN chmod +x /tmp/your_ros2_installer.sh

# Run the installer script with bash -e to stop on errors
RUN bash -e /tmp/your_ros2_installer.sh

# Source the ROS 2 setup script for all users
RUN echo "source /opt/ros/jazzy/setup.bash" >> /etc/bash.bashrc

# Set the default command to start a bash shell
CMD ["/bin/bash"]
