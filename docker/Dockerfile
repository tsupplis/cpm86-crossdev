FROM alpine:latest
RUN apk update && \
    apk -q add '!usr-merge-nag' 2> /dev/null || true && \
    apk -q upgrade --prune || true && \
    apk add \
          autoconf \
          automake \
          bash \
          bison \
          cmake \
          coreutils \
          curl \
          diffutils \
          dos2unix \
          file \
          g++ \
          gcc \
          gettext \
          git \
          gzip \
          iputils \
          libtool \
          make \
          ncurses \
          ncurses-dev \
          patch \
          shadow \
          sudo \
          tar \
          unzip \
          vim \
          wget2 \
          xz
COPY docker/arch /usr/bin/arch
COPY .. /opt/crossdev
RUN (ln -fs "$(command -v wget2)" "$(command -v wget)" && cd /opt/crossdev && ./clear_tools && ./fetch_tools)
ARG USER_ID="8855"
ARG USER="cpm86"
ARG HOME="/home"
RUN useradd -M ${USER} -u ${USER_ID} -d ${HOME}
USER ${USER_ID}
ENV USER="${USER}"
ENV USERNAME="${USER}"
ENV LOGNAME="${USER}"
ENV PATH="/opt/crossdev/bin:${PATH}"
CMD ["/bin/bash", "--login"]
