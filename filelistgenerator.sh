#!/bin/bash

# Set the directory where the files are located
directory="/gpfs/mnt/gpfs02/sphenix/user/bade/sphenix/work/g4eval/DST_TPC/ana478_29052025/"

for runnumber in 53079 53080 53081 53194 53195 53196 53197 53494 53513 53517 53534 53530 53531 53532 53630 53631 53632 53652 53686 53687 53741 53742 53743 53744 53876 53877 53879; do 

    output_file="file_lists/file_list_${runnumber}_ana478_29052025.txt"
    > "$output_file"

    find "$directory" -type f -name "dst_eval-000${runnumber}*" >> "$output_file"

    echo "$output_file created with run numbers."
done
