# Multistage docker build, requires docker 17.05

# builder stage
FROM ubuntu:16.04 as builder

RUN apt-get update && \
    apt-get --no-install-recommends --yes install \
        ca-certificates \
        cmake \
        g++ \
        libboost1.58-all-dev \
        libssl-dev \
        libzmq3-dev \
        libreadline-dev \
        libsodium-dev \
        make \
        pkg-config \
        graphviz \
        doxygen \
        git

WORKDIR /src
COPY . .

ARG NPROC
RUN rm -rf build && \
    if [ -z "$NPROC" ];then make -j$(nproc) release-static;else make -j$NPROC release-static;fi

# runtime stage
FROM ubuntu:16.04

RUN apt-get update && \
    apt-get --no-install-recommends --yes install ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt

COPY --from=builder /src/build/release/bin/* /usr/local/bin/

# Contains the blockchain
VOLUME /root/.sumokoin

# Generate your wallet via accessing the container and run:
# cd /wallet
# sumo-wallet-cli
VOLUME /wallet

EXPOSE 19733
EXPOSE 19734

ENTRYPOINT ["sumokoind", "--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=19733", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=19734", "--non-interactive"]
