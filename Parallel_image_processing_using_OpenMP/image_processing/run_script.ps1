# Compile the program with OpenMP
gcc -o output main.c -fopenmp -lm

# Function to run the program with a specific number of cores
function RunWithCores($numCores) {
    Write-Host "Running with $numCores core(s)"
    $afterTime = Measure-Command { Start-Process -FilePath .\output.exe -ArgumentList $numCores -Wait } | Select-Object -ExpandProperty TotalMilliseconds
    Write-Host "Execution time with $numCores core(s): $($afterTime) milliseconds"
    Write-Host "###################################`n"
}

# Run the program with 1, 2, 4, and 8 cores
RunWithCores 1
RunWithCores 2
RunWithCores 4
RunWithCores 8

