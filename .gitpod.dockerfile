FROM gitpod/workspace-full

RUN sudo apt-get update  && sudo apt-get install -y libsdl.2-dev
