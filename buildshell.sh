#!/bin/bash

exec docker run -it --rm \
     -v $(pwd):/source \
     -w /source \
     --network=host \
     luksamuk/psxtoolchain:latest \
     /bin/bash
