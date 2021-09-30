FROM alpine:latest
RUN apk update && \
    apk  upgrade && \
    apk  add \
        iputils \
        make \
        texinfo \
        file \
        gettext \
        ansible \
        wget \
        ncurses \
        apache-ant \
        autoconf \
        automake \
        bash \
        bison \
        gcc \
        g++ \
        curl \
        flex \
        gdb \
        ncurses-dev \
        openjdk8 \
        py3-pip \
        py-virtualenv \
        shadow \
        sqlite-dev \
        sudo \
        sysstat \
        tcpdump \
        nodejs \
        vim
COPY arch /usr/bin/arch
RUN (cd /opt;git clone https://github.com/tsupplis/cpm86-crossdev crossdev)
RUN (cd /opt/crossdev;./fetch_tools)
RUN groupmod -n staffosx dialout
ARG USER_ID=0
ARG USER=code_executor
ARG HOME=/
RUN useradd -M ${USER} -u ${USER_ID} -g staffosx -d ${HOME}
RUN echo ${USER} 'ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
USER ${USER_ID}
ENV USER ${USER}
ENV USERNAME ${USER}
ENV LOGNAME ${USER}
CMD /bin/bash --login
