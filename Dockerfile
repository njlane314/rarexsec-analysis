FROM ghcr.io/uboone/analysis-base:latest

SHELL ["/bin/bash", "-lc"]

# Make UPS-based environment available on shell startup
COPY .setup.sh /etc/profile.d/rarexsec.sh
RUN chmod +x /etc/profile.d/rarexsec.sh

ENV LD_LIBRARY_PATH=/workspace/build/framework:$LD_LIBRARY_PATH \
    ROOT_INCLUDE_PATH=/workspace/build/framework:$ROOT_INCLUDE_PATH

WORKDIR /workspace
