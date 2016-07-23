param(
    [string] $csvfile = $(throw "please provide the path to a cputarget sweep result file"),
    [switch] $zerointercept = $false,
    [int] $sigfigs = 5
    )

function get-sigfigs(
    [decimal]$value,
    [int]$sigfigs
    )
{
    $log = [math]::Ceiling([math]::log10([math]::abs($value)))
    $decimalpt = $sigfigs - $log

    # if all sigfigs are above the decimal point, round off to
    # appropriate power of 10
    if ($decimalpt -lt 0) {
        $pow = [math]::Abs($decimalpt)
        $decimalpt = 0
        $value = [math]::Round($value/[math]::Pow(10,$pow))*[math]::pow(10,$pow)
    }

    "{0:F$($decimalpt)}" -f $value
}

write-host -ForegroundColor Green CPU Target Sweep Report`n
write-host -ForegroundColor Green The following equations and coefficients are the linear
write-host -ForegroundColor Green fit to the measured results at the given write ratios.
write-host -ForegroundColor Cyan ("-"*20)

write-host -ForegroundColor Yellow NOTE: take care that these formula are only used to reason about
write-host -ForegroundColor Yellow "      " the region where these values are in a linear relationship.
write-host -ForegroundColor Yellow "       In particular, at high AVCPU the system may be saturated."
write-host -ForegroundColor Yellow "Use R^2 (coefficient of determination) as a quality check for the fit."
write-host -ForegroundColor Yellow "Values close to 100% mean that the data is indeed linear. If R2 is"
write-host -ForegroundColor Yellow "significantly less than 100%, a closer look at system behavior may"
write-host -ForegroundColor Yellow "be required."

if ($zerointercept) {
    write-host -ForegroundColor Red NOTE: forcing to a "(AVCPU=0,IOPS=0)" intercept may introduce error
} else {
    write-host -ForegroundColor Red "NOTE: with a non-zero constant coefficient, care should be used at`nlow AVCPU that the result is meaningful"
}

# do the check fit of QoS to IOPS
# this will let us check for non-CPU limited saturation (poor r2 is a giveaway)
# we can have an excellent CPU->IOPS fit but not actually have been able to stress in much CPU
# and have lots of repeated measurements as we tried to step up QoS
$h = @{}
get-linfit -csvfile $csvfile -xcol QOS -ycol IOPS -idxcol WriteRatio -zerointercept:$zerointercept |% {
    $h[$_.Key] = $_
}

# now fit IOPS to Average CPU
get-linfit -csvfile $csvfile -xcol AVCPU -ycol IOPS -idxcol WriteRatio -zerointercept:$zerointercept | sort -Property Key |% {

    write-host -ForegroundColor Cyan ("-"*20)
    write-host $_.Key

    if ($zerointercept) {
        write-host ("{0} = {1}({2})" -f $_.Y,$(get-sigfigs $_.B $sigfigs),$_.X)
    } else {

        if ($_.A -ge 0) {
            $sign = "+"
        } else {
            $sign = ""
        }

        write-host ("{0} = {1}({2}){4}{3}" -f $_.Y,$(get-sigfigs $_.B $sigfigs),$_.X,$(get-sigfigs $_.A $sigfigs),$sign)
    }

    write-host ("N = {1}`nR^2 goodness of fit {0:P2}" -f $_.R2,$_.N)
    if ($h[$_.Key].R2 -le 0.5) {
        write-host -ForegroundColor Yellow "WARNING: for $($_.Key) it does not appear that IOPS moved in"
        write-host -ForegroundColor Yellow "`trelation to attempts to raise the QoS limit. Check if the"
        write-host -ForegroundColor Yellow "`tsystem is storage limited for this mix."
    }
}