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
          screen \
          shadow \
          sudo \
          tar \
          unzip \
          vim \
          wget2 \
          xz
COPY docker/arch /usr/bin/arch
COPY .. /opt/crossdev
ARG USER_ID=8855
ARG USER="cpm86"
ENV USER="${USER}"
ARG HOME="/home/${USER}"
ENV USERNAME="${USER}"
ENV LOGNAME="${USER}"
ENV PATH="/opt/crossdev/bin:${PATH}"
RUN useradd -M ${USER} -u ${USER_ID} -d ${HOME}
CMD ["/bin/bash", "--login"]
RUN chown -R "${USER}":"${USER}" /opt && \
    ln -fs "$(command -v wget2)" "$(command -v wget)" && \
    mkdir -p "${HOME}"/.screen && \
    chown -R "${USER}":"${USER}" "${HOME}" && \
    chmod 700 "${HOME}"/.screen
USER ${USER_ID}
RUN set -e; \
    ( \
        cd /opt/crossdev && \
        ./clear_tools && \
        ./fetch_tools \
    ) || { \
        rc=$?; \
        cd /opt/crossdev; \
        echo "=== FETCH TOOLS FAILED (exit $rc) ==="; \
        for f in fetch-*.log; do \
            [ -f "$f" ] || continue; \
            echo "--- $f ---"; \
            cat "$f"; \
            echo; \
        done; \
        exit "$rc"; \
    }
