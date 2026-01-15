# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libgumbo-dev \
    libsqlite3-dev \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build application
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release -j$(nproc) \
    && cmake --install build --prefix /app

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libgumbo1 \
    libsqlite3-0 \
    libssl3 \
    cron \
    && rm -rf /var/lib/apt/lists/*

# Create application user
RUN useradd -m -u 1000 -s /bin/bash bluray

# Set working directory
WORKDIR /app

# Copy binary from builder
COPY --from=builder /app/bin/bluray-tracker /app/bluray-tracker

# Copy crontab configuration
COPY docker-crontab /etc/cron.d/bluray-tracker-cron
RUN chmod 0644 /etc/cron.d/bluray-tracker-cron \
    && crontab -u bluray /etc/cron.d/bluray-tracker-cron

# Create necessary directories
RUN mkdir -p /app/cache /app/data \
    && chown -R bluray:bluray /app

# Switch to application user
USER bluray

# Expose web port
EXPOSE 8080

# Volume for persistent data
VOLUME ["/app/data", "/app/cache"]

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/ || exit 1

# Default command: run web server
CMD ["/app/bluray-tracker", "--run", "--db", "/app/data/bluray-tracker.db", "--port", "8080"]
