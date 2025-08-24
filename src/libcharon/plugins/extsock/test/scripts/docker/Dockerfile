# Dockerfile for extsock Plugin Test Environment
# TASK-016: CI/CD Pipeline Docker Support
# 
# This container provides a consistent testing environment
# for the extsock plugin test suite across different platforms.

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    gcc \
    clang \
    make \
    cmake \
    pkg-config \
    \
    # Testing frameworks
    libcheck-dev \
    \
    # Memory and performance tools  
    valgrind \
    time \
    \
    # Code analysis tools
    cppcheck \
    clang-tidy \
    lcov \
    gcov \
    \
    # Utilities
    git \
    curl \
    wget \
    tree \
    vim \
    less \
    \
    # Cleanup
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create test user (non-root for security)
RUN useradd -m -u 1000 -s /bin/bash testuser && \
    mkdir -p /workspace && \
    chown -R testuser:testuser /workspace

# Set up working directory
WORKDIR /workspace

# Copy test suite files
COPY --chown=testuser:testuser . .

# Switch to test user
USER testuser

# Verify environment
RUN echo "=== Build Environment Verification ===" && \
    gcc --version && \
    echo "Check framework:" && \
    pkg-config --exists check && echo "✅ Check framework available" || echo "❌ Check framework missing" && \
    echo "Valgrind:" && \
    valgrind --version | head -1 && \
    echo "=== Environment Ready ==="

# Set default command
CMD ["bash"]

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD gcc --version || exit 1

# Labels
LABEL maintainer="extsock-test-team" \
      version="1.0" \
      description="extsock Plugin Test Environment" \
      build-date="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"

# Environment variables for testing
ENV TEST_ENVIRONMENT=docker \
    BUILD_TYPE=Release \
    PARALLEL_JOBS=2