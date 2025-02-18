# Use a minimal base image with build essentials
FROM debian:latest

# Install required dependencies (GCC, libcurl, and runtime utilities)
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    curl \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy the C source file into the container
COPY bw_alert.c .

# Compile the C program
RUN gcc -o bwalert bw_alert.c -lcurl

# Expose any required ports (not necessary for this use case)
# EXPOSE 8080

# Command to run the program (can be overridden at runtime)
CMD ["./bwalert", "eth0", "1", "50", "172.17.0.3,192.168.1.100", "10.10.10.1"]

