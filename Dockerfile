# ---- build: sentry-native + sample, with debug info -------------------------
FROM debian:trixie AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential cmake curl ca-certificates unzip libcurl4-openssl-dev zlib1g-dev binutils \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
ARG SENTRY_NATIVE=0.17.4
RUN curl -sL -o sn.zip https://github.com/getsentry/sentry-native/releases/download/${SENTRY_NATIVE}/sentry-native.zip \
    && mkdir -p external/sentry-native && unzip -q sn.zip -d external/sentry-native && rm sn.zip
COPY CMakeLists.txt main.c ./
RUN cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build --parallel
# split debug info out, ship stripped binaries
RUN mkdir -p /out /symbols \
    && cp build/sample build/crashpad_build/handler/crashpad_handler build/libsentry.so /out/ 2>/dev/null \
     || cp build/sample $(find build -name crashpad_handler -type f) $(find build -name 'libsentry.so*' -type f | head -1) /out/ \
    && cd /out && for f in *; do cp $f /symbols/$f; strip --strip-debug $f; done
# source bundle: main.c next to the debug info, so Sentry shows code lines too
RUN curl -sL -o /usr/local/bin/sentry-cli       https://github.com/getsentry/sentry-cli/releases/latest/download/sentry-cli-Linux-x86_64     && chmod +x /usr/local/bin/sentry-cli     && sentry-cli debug-files bundle-sources /symbols/sample

# ---- symbols: CI exports this stage and uploads it to Sentry -----------------
FROM scratch AS symbols
COPY --from=build /symbols /

# ---- runtime -------------------------------------------------------------------
FROM debian:trixie-slim
ARG RELEASE=dev
ENV RELEASE=${RELEASE}
RUN apt-get update && apt-get install -y --no-install-recommends libcurl4t64 python3 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /out/ ./
COPY trigger.py .
ENV LD_LIBRARY_PATH=/app
EXPOSE 8080
CMD ["python3", "trigger.py"]
