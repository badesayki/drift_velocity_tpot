#!/bin/bash

# Set the directory where the files are located
directory="/cvmfs/sphenix.sdcc.bnl.gov/calibrations/sphnxpro/cdb/TPC_LAMINATION_FIT_CORRECTION/*/*"

# Create or recreate the output 
output_file="full_caliblist.txt"
> "$output_file"
echo "run number, path" >> "$output_file"
# Loop through the matching files in the directory
for file in $directory/*.root; do
    # Extract the run number using basename and pattern matching
    filename=$(basename "$file")
    run_number=${filename#*-000} #This works because we only have 5 digit run numbers for now. It'll break once we get to 6 digits. Dont forget to fix that
    run_number=${run_number%.root}
    echo "$run_number"

    # Write the run number to output file
    echo "$run_number, $file" >> "$output_file"
done

echo "$output_file created with run numbers."
