#!/bin/bash
PRG="$0"
while [ -h "$PRG" ] ; do
  ls=`ls -ld "$PRG"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '/.*' > /dev/null; then
    PRG="$link"
  else
    PRG=`dirname "$PRG"`/"$link"
  fi
done
BASE=$(cd $(dirname $PRG); pwd)

DOCKER=$(which docker)
if [ -z "$DOCKER" ] ;then
  echo "docker is required ..."
  exit 1
fi
$DOCKER run -d -p 6080:80 --entrypoint=/startup.sh  --privileged=true -v ${BASE}:/root/workspace/DiyOS --workdir /root/workspace/DiyOS --name diyos_dev diyos_dev:latest
echo "diyos dev container startup!!!"
echo "http://127.0.0.1:6080"
