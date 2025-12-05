FROM rust:1.91.1-slim-trixie

RUN apt update && apt install -y wget mingw-w64 zip bzip2 git gcc g++ libsdl3-dev make && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY . .
RUN cd /build && make clean
RUN mkdir /build/artifacts

# Windows dependencies
RUN rustup target add x86_64-pc-windows-gnu
RUN cd /build && wget --quiet https://github.com/libsdl-org/SDL/releases/download/release-3.2.28/SDL3-devel-3.2.28-mingw.tar.gz && tar xvzf SDL3-devel-3.2.28-mingw.tar.gz
RUN mv /build/SDL3-3.2.28 /build/SDL3
RUN cp /build/SDL3/x86_64-w64-mingw32/bin/SDL3.dll /build
RUN cp /usr/lib/gcc/x86_64-w64-mingw32/14-posix/libgcc_s_seh-1.dll /build
RUN cp /usr/lib/gcc/x86_64-w64-mingw32/14-posix/libstdc++-6.dll /build
RUN cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll /build

CMD ["bash"]
