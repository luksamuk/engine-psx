FROM luksamuk/psxtoolchain:latest
RUN apt update
RUN apt install -y \
    python-is-python3 \
    python3-numpy \
    python3-shapely \
    python3-pandas

# RUN apt install -y libxcb-xinerama0 libxcb-cursor0 xvfb

# Build and install tiled 1.11.2
RUN apt install -y \
    qtbase5-dev \
    qtdeclarative5-dev \
    libqt5svg5-dev \
    qttools5-dev-tools \
    zlib1g-dev \
    zlib1g-dev \
    libgl1-mesa-dev \
    build-essential \
    python3-dev \
    qbs
RUN cd / && \
    git clone --depth 1 --branch v1.11.2 https://github.com/mapeditor/tiled
RUN cd tiled &&\
    qbs setup-toolchains --detect &&\
    qbs config profiles.default.cpp.toolchainInstallPath /usr/bin &&\
    qbs config profiles.default.cpp.compilerName g++
ENV PYTHONHOME=/usr
RUN cd tiled &&\
    qbs install qbs.installPrefix:"/usr"
RUN cd tiled &&\
    qbs install --no-build --install-root /
ENV QT_QPA_PLATFORM=offscreen
RUN tiled --version && rm -r /tiled

RUN apt clean &&\
    rm -rf /var/lib/apt/lists/*

ENV HOME="/home/builduser/"

# Enable and install Tiled python/csv plugins
RUN mkdir -p ~/.config/mapeditor.org &&\
    mkdir -p ~/.tiled &&\
    printf "[Plugins]\nEnabled=libpython.so\n" \
    > ~/.config/mapeditor.org/tiled.conf

COPY ./assets/tiled /usr/bin/tiled
COPY ./tools/tiled_exporter/chunkexporter.py ~/.tiled/chunkexporter.py
COPY ./tools/tiled_exporter/lvlexporter.py ~/.tiled/lvlexporter.py

RUN tiled --export-formats &&\
    cat ~/.config/mapeditor.org/tiled.conf &&\
    false

# Prevent errors with CMake and Tiled
ENV CMAKE_MAKE_PROGRAM=make

COPY . /sonicxa
WORKDIR /sonicxa
RUN make purge
RUN make cook
RUN make elf
RUN make iso
RUN make chd

