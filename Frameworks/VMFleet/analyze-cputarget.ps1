param(
    [string]$csvfile = $(throw "please provide the path to a cputarget sweep result file"),
    [switch]$zerointercept = $false
    )

write-host -ForegroundColor Green CPU Target Sweep Report`n
write-host -ForegroundColor Green The following equations and coefficients are the linear
write-host -ForegroundColor Green fit to the measured results at the given write ratios.
write-host -ForegroundColor Cyan ("-"*20)

write-host -ForegroundColor Yellow NOTE: take care that these formula are only used to reason about
write-host -ForegroundColor Yellow "      " the region where these values are in a linear relationship.
write-host -ForegroundColor Yellow "       In particular, at high AVCPU the system may be saturated."
write-host -ForegroundColor Yellow "Use R2 (coefficient of determination) as a quality check for the fit."
write-host -ForegroundColor Yellow "Values close to 100% mean that the data is indeed linear. If R2 is"
write-host -ForegroundColor Yellow "significantly less than 100%, a closer look at system behavior may"
write-host -ForegroundColor Yellow "be required."

if ($zerointercept) {
    write-host -ForegroundColor Red NOTE: forcing to a "(AVCPU=0,IOPS=0)" intercept may introduce error
} else {
    write-host -ForegroundColor Red "NOTE: with a non-zero constant coefficient, care should be used at`nlow AVCPU that the result is meaningful"
}

$results = import-csv -Path $csvfile -Delimiter "`t" | group -Property WriteRatio

$results |% {

    write-host -ForegroundColor Cyan ("-"*20)
    write-host "Write Ratio: $($_.Name)"

    get-linfit -csvfile $csvfile -xcol AVCPU -ycol IOPS -idxcol WriteRatio -idxval $_.Name -zerointercept:$zerointercept
}