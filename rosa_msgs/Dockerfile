ARG UBT_3RD_IMAGE
ARG ROSA_IMAGE
ARG BASE_IMAGE
ARG ROSA_GENERATE_TOOL_IMAGE

FROM $UBT_3RD_IMAGE as ubt_3rd_image
FROM $ROSA_IMAGE as rosa_image
FROM $BASE_IMAGE as base_image

USER root

COPY . .

ENV ROSA_VERSION=2
RUN --mount=type=ssh \
  --mount=type=bind,from=ubt_3rd_image,source=/opt/ubt_3rdparty,target=/opt/ubt_3rdparty,rw \
  --mount=type=bind,from=rosa_image,source=/opt/rosa,target=/opt/rosa,rw \
  source /opt/rosa/setup.bash && \
  colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release
