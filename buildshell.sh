#!/bin/bash

exec docker run -it --rm \
     -v $(pwd):/source \
     -v $(pwd):$(pwd) \
     -w /source \
     --network=host \
     --add-host localhost:host-gateway \
     luksamuk/psxtoolchain:latest \
     /bin/bash
