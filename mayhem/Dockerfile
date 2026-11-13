# syntax=docker/dockerfile:1.7
#
# Commit image for Melon: builds on the org C/C++ base, which provides clang, libFuzzer and the
# build-contract ENV (SANITIZER_FLAGS, DEBUG_FLAGS, LIB_FUZZING_ENGINE, STANDALONE_FUZZ_MAIN, SRC).
#
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t melon .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
# Never bake SOURCE_DATE_EPOCH as an ENV: an empty value breaks clang. build.sh unsets it when empty.
ARG SOURCE_DATE_EPOCH
# Declared without a default so the base ENV flows through unless --build-arg overrides it.
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

# Optional compile-cache pre-fill (SPEC 6.2 item 18); older bases fall back to the plain build.
RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

# A failing suite fails the image build.
RUN bash mayhem/test.sh

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
