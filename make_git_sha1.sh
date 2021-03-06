#!/bin/bash

OUTFILE=$1
GIT_SHA1=`git describe --always --dirty --abbrev=40 --match=NeVeRmAtCh`

cat > ${OUTFILE}.tmp << EOP
#define THE_GIT_SHA1 "${GIT_SHA1}"

static const char GIT_SHA1[] = THE_GIT_SHA1;

const char *get_git_sha1()
{
    return GIT_SHA1;
}
EOP

if [ -e ${OUTFILE} ]
then
  diff ${OUTFILE} ${OUTFILE}.tmp --brief > /dev/null
  CHANGED=$?
  if [ ${CHANGED} -eq 0 ]
  then
    # unchanged
    rm -f ${OUTFILE}.tmp
    exit
  fi
fi

mv -f ${OUTFILE}.tmp ${OUTFILE}
rm -f ${OUTFILE}.tmp
