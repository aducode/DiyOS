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
$DOCKER build -t diyos_dev:latest ${BASE}
