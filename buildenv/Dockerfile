FROM randomdude/gcc-cross-x86_64-elf

RUN apt-get update && apt-get install -y \
    nasm \
    grub-pc-bin \
    grub-common \
    xorriso \
    && rm -rf /var/lib/apt/lists/*

VOLUME /root/env
WORKDIR /root/env
