#!/bin/bash

exec docker run -it --rm \
     -v $(pwd):/source \
     -w /source \
     luksamuk/psxtoolchain:latest \
     /bin/bash
