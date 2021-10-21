FROM alpine:latest
RUN apk update && \
    apk  upgrade && \
    apk  add \
        iputils \
        make \
        file \
        gettext \
        wget \
        ncurses \
        bash \
        bison \
        gcc \
        g++ \
        git \
        curl \
        unzip \
        patch \
        diffutils \
        dos2unix \
        ncurses-dev \
        shadow \
        sudo \
        vim
COPY arch /usr/bin/arch
RUN (cd /opt;git clone https://github.com/tsupplis/cpm86-crossdev crossdev)
RUN (cd /opt/crossdev;./fetch_tools)
ARG USER_ID=8855
ARG USER=cpm86
ARG HOME=/home
RUN useradd -M ${USER} -u ${USER_ID} -d ${HOME}
USER ${USER_ID}
ENV USER ${USER}
ENV USERNAME ${USER}
ENV LOGNAME ${USER}
ENV PATH="/opt/crossdev/bin:${PATH}"
CMD /bin/bash --login
