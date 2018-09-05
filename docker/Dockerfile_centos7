############################################################
# Dockerfile that builds and runs dependant docker files

FROM usd-docker/usd:centos7-usd-0.7.5

MAINTAINER Animal Logic
COPY ./ /tmp/usd-build/AL_USDMaya
COPY ./docker/build_alusdmaya.sh /tmp/
RUN /tmp/build_alusdmaya.sh

# Setup the environment using the location where AL_USDMaya is installed
CMD echo "-Updating PATH-"
ENV PATH $BUILD_DIR/src:$PATH

CMD echo "-Updating PYTHONPATH-"
ENV PYTHONPATH $BUILD_DIR/lib/python:$PYTHONPATH

CMD echo "-Updating LD_LIBRARY_PATH-"
ENV LD_LIBRARY_PATH $BUILD_DIR/lib:$LD_LIBRARY_PATH

CMD echo "-Updating MAYA_PLUG_IN_PATH-"
ENV MAYA_PLUG_IN_PATH $BUILD_DIR/plugin:$MAYA_PLUG_IN_PATH

CMD echo "-Updating MAYA_SCRIPT_PATH-"
ENV MAYA_SCRIPT_PATH $BUILD_DIR/lib:$BUILD_DIR/share/usd/plugins/usdMaya/resources:$MAYA_SCRIPT_PATH

CMD echo "-Updating PXR_PLUGINPATH_NAME-"
ENV PXR_PLUGINPATH_NAME $BUILD_DIR/share/usd/plugins:$PXR_PLUGINPATH_NAME
