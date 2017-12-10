import os
import subprocess

inputModes = ['iidx', 'ij']
epsValues = [0.3, 0.4, 0.5, 0.7, 0.9]
kValues = [10, 50, 100]
inputFiles = ['wiki1.csr', 'wiki2.csr']
outputFile = ('%s.%s.%s.%s.csr')

os.chdir('C:/Users/sends/ExtraCredit-Workspace/ExtraCredit/data/')
command = ('../build/findsim -m %s -eps %f -k %d %s %s')
 
timesFile = open('times.csv', 'w')
lines = []
 
for mode in inputModes:
         
    if (mode == 'ij'):
        displayMode = 'IdxJoin'
    else:
        displayMode = 'FindSim'
    print ('\n==== Algorithm: ' + displayMode + ' ====')
             
    for inputFile in inputFiles:
     
        print ('\nData Set: ' + str(inputFile))
        inputFilename = inputFile.split('.')
         
        for eps in epsValues:
            for k in kValues:
                 
                outputFile_expanded = outputFile % (inputFilename[0], mode, eps, k )
                #Execute the findsim command
                cmd = command % (mode, eps, k, inputFile, outputFile_expanded)
 
                output = subprocess.check_output(cmd, shell=True)
                searchTime = output.split('Similarity search:  ')[1]
                #Extract the time with the 4 significant digits format
                time = searchTime.split(' ')[0]
                 
                lines.append(str(displayMode) + ',' + str(inputFile) + ',' + str(eps) + ',' + str(k) + ',' + str(time))
                print ('eps: '  + str(eps) + ' k: ' + str(k) + ' Time: ' + str(time))
 
#Write timing results to output CSV file
timesFile.write('\n'.join(lines))
timesFile.close()
print ('\n**** Execution times written to file ****\n')

#Compare the output files from each of the algorithm for every input
compareFile1 = ('%s.ij.%s.%s.csr')
compareFile2 = ('%s.iidx.%s.%s.csr')
inputFiletype = ['wiki1', 'wiki2']

command = ('../build/findsim -mode eq %s %s')

print ("\n===== Comparing the output files from IdxJoin and FindSim Algorithms ====")
for fileType in inputFiletype:
    
    for eps in epsValues:
        for k in kValues:
                
            compareFile1_expanded = compareFile1 % (fileType, eps, k)
            compareFile2_expanded = compareFile2 % (fileType, eps, k)
            
            #Execute compare command
            cmd = command % (compareFile1_expanded, compareFile2_expanded)
            
            output = subprocess.check_output(cmd, shell=True)
            #Extract the differences summary between the 2 files
            outputDifference = output.split('Differences: ')[1]
           
            print ('\nIdxJoin output file: ' + compareFile1_expanded )
            print ('FindSim output file: ' + compareFile2_expanded)
            print ('\nDifferences:' + str(outputDifference))