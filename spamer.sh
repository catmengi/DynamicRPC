#!/bin/bash

for i in {1..256}
do
   ./testc & 
done
#pkill -f testc
