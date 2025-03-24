#!/bin/sh

case "$1" in
    start)
		echo "Starting aesdsocket daemon"
		start-stop-daemon -s -n aesdoscket -d /usr/bin/aesdoscket
		;;
	stop)
		echo "Stopping aesdsocket daemon"
		start-stop-daemon -k -n aesdoscket
		;;
	*)
	echo "Usage: $0 {start|stop}"
	exit 1
esac
exit 0
