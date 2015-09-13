#!/bin/bash

 sudo phpize --clean
 sudo phpize
 sudo ./configure --with-iocron --enable-iocron-debug
 sudo make
 sudo make install
 
 lldb -- php -S 127.0.0.1:8000 -t /Users/ilk3r/PhpstormProjects/test/ --