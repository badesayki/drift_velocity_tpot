#!/bin/bash

# Set the directory where the files are located
directory="/sphenix/lustre01/sphnxpro/production/run2pp/physics/ana478_nocdbtag_v001/DST_STREAMING_EVENT_TPOT/*/*"

# Create or recreate the output 
output_file="full_runlist.txt"
> "$output_file"

# Loop through the matching files in the directory
for file in $directory/DST_STREAMING_EVENT_TPOT_run2pp_ana478_nocdbtag_v001-*-00000.root; do
    # Extract the run number using basename and pattern matching
    echo "$file"
    filename=$(basename "$file")
    echo "$filename"
    run_number=${filename#DST_STREAMING_EVENT_TPOT_run2pp_ana478_nocdbtag_v001-000} #This works because we only have 5 digit run numbers for now. It'll break once we get to 6 digits. Dont forget to fix that
    run_number=${run_number%-00000.root}
    echo "$run_number"

    # Write the run number to output file
    echo "$run_number" >> "$output_file"
done

echo "$output_file created with run numbers."
