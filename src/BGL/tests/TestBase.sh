#!/bin/bash

base=`basename $0 .sh`

if [ "${base}" = "TestBase" ] ; then
    echo "This script isn't run directly. You make softlinks to it with the test names, and run it from those."
    exit 73
fi

export TIMEOUT=30

groupnum=`echo ${base} | sed 's/^.*G\([0-9]*\)[^0-9].*$/\1/'`
testnum=`echo ${base} | sed 's/^.*T\([0-9]*\)[^0-9]*$/\1/'`
subtest=`echo ${base} | sed 's/^.*T[0-9]*\([^0-9]\).*$/\1/'`

TESTROOT="G${groupnum}_T${testnum}"

EXPECTEDDIR="expected_svgs"
RESULTDIR="result_svgs"
LOGDIR="logs"

alldirs="${EXPECTEDDIR} ${RESULTDIR} ${LOGDIR}"
for d in ${alldirs} ; do
    if [ ! -d $d ]; then
	mkdir -p $d
    fi
done

SVGPAT="${TESTROOT}_*_${subtest}_*.svg"

LOGFILE="${LOGDIR}/Log-${TESTROOT}.txt"

TESTBIN="./${TESTROOT}_*"
${TESTBIN} > ${LOGFILE} 2>&1 &
PRODPID=$!

export LOGFILE
export PRODPID
export MAINPID=$$

killitnow() {
    ps -p $2 | grep -v "PID TTY" >> /dev/null 2>&1
    if [ $? == 0 ] ; then
	kill -$1 $2
	wait $2 2>/dev/null
    fi
}


exit_timeout() {
    echo "Timed Out." >> ${LOGFILE}
    killitnow SIGTERM ${PRODPID}
    exit -1
}
trap exit_timeout SIGUSR1


die_die_die() {
    echo "Killed by user." >> ${LOGFILE}
    killitnow SIGTERM ${PRODPID}
    exit -1
}
trap die_die_die SIGINT



( trap 'exit 0' TERM ; sleep ${TIMEOUT} ; killitnow SIGUSR1 ${MAINPID} ) &
TPID=$!
export TPID


wait ${PRODPID}
resval=$?

kill $TPID

if [ $resval -ne 0 ] ; then
    exit -1
fi

EXPECTEDSVG=`ls -1 ${EXPECTEDDIR}/${SVGPAT}`
RESULTSSVG=`ls -1 ${RESULTDIR}/${SVGPAT}`

if [ ! -e "${EXPECTEDSVG}" ] ; then
    exit -1
fi

if [ ! -e "${RESULTSSVG}" ] ; then
    exit -1
fi

cmp -s "${EXPECTEDSVG}" "${RESULTSSVG}"
if [ $? -ne  0 ]; then
    exit -1
fi
exit 0

