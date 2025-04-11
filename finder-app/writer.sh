#!/bin/bash

#Make sure the arguments are being called correctly...
if [ $# -ne 2 ]; then
    echo "Error: Missing arguments. Usage: $0 <writefile> <writestr>"
    exit 1
fi

writefile=$1
writestr=$2

# Create directory if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Write the string to the file. Echo overwrites...
echo "$writestr" > "$writefile"

# Check if the file was created successfully
if [ ! -f "$writefile" ]; then
    echo "Error: Could not create file $writefile"
    exit 1
fi

exit 0
