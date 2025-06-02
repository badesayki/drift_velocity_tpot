#!/bin/csh

source ~/sphenix_setup.csh

# process arguments
set nEvents = ${1}
set runNumber = ${2}
set job_offset = ${3}
set job_raw = ${4}
@ job_id = ($job_raw - $job_offset)
@ segment = ($job_id / 10 )
@ skip = ($job_id - 10 * $segment) * 1000

set tag = "ana478_nocdbtag_v001"

echo "run_Fun4All_DriftAnalysis - tag: ${tag}"
echo "run_Fun4All_DriftAnalysis - runNumber: ${runNumber}"
echo "run_Fun4All_DriftAnalysis - segment: ${segment}"
echo "run_Fun4All_DriftAnalysis - nEvents: ${nEvents}"
echo "run_Fun4All_DriftAnalysis - skip: ${skip}"

# output path
set output_path = "DST/"

# output file
set outputfile = `printf "${output_path}/dst_eval-%08i-%04i-%04i-full.root" $runNumber $segment $skip`
echo "run_Fun4All_DriftAnalysis - outputfile: ${outputfile}"

# residual file
set residualfile = `printf "${output_path}/TrackResiduals-%08i-%04i-%04i-full.root" $runNumber $segment $skip`
echo "run_Fun4All_DriftAnalysis - residualfile: ${residualfile}"

# make sure output path exists
mkdir -p ${output_path}

# check output file existence
if ( -f ${outputfile} ) then
  echo "run_Fun4All_DriftAnalysis - file ${outputfile} exists. aborting."
  exit 0
endif

touch ${outputfile}

# start root, load macro and exit
echo "running root"
root -b << EOF
.L Fun4All_DriftAnalysis.C
Fun4All_DriftAnalysis( $nEvents, $skip, "${tag}", $runNumber, $segment, "${outputfile}", "${residualfile}" )
EOF
