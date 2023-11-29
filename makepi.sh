#!/bin/bash
if [ "$1" == "" ]; then
	docker run -it --rm -v `pwd`:/build mitchallen/pi-cross-compile
elif [ "$1" == "rm" ]; then
	docker rm pibuild
else
	docker run --name pibuild -it --rm -v `pwd`:/build mitchallen/pi-cross-compile bash
fi
