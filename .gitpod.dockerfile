FROM gitpod/workspace-full

RUN sudo apt-get update  && sudo apt-get install -y libsdl2-dev libsdl2-2.0-0
