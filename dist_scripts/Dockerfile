FROM rust:1.85.1-slim-bookworm

RUN apt update && apt install -y wget mingw-w64 zip bzip2 git gcc g++ libsdl2-dev make && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY . .
RUN cd /build && make clean
RUN mkdir /build/artifacts

# Windows dependencies
RUN rustup target add x86_64-pc-windows-gnu
RUN cd /build && wget --quiet https://github.com/libsdl-org/SDL/releases/download/release-2.28.3/SDL2-devel-2.28.3-mingw.tar.gz && tar xvzf SDL2-devel-2.28.3-mingw.tar.gz
RUN mv /build/SDL2-2.28.3 /build/SDL2
RUN cp /build/SDL2/x86_64-w64-mingw32/bin/SDL2.dll /build
RUN cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libgcc_s_seh-1.dll /build
RUN cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libstdc++-6.dll /build
RUN cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll /build

CMD ["bash"]
