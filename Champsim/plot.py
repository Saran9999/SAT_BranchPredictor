import sys
import os
import numpy as np
import tkinter
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

trace = sys.argv[1]
trace += ".xz"
N_warm = sys.argv[2]
N_sim = sys.argv[3]


def find_Accuracy(file, nsim):
    search_ipc = "cumulative IPC:"
    ipc = None
    folder = 'results_' + str(nsim) + 'M'
    accuracy = None
    search_accuracy = "MPKI:"
    with open("./%s/%s"%(folder, file), 'rt') as fin:
        for line in fin:
            found = True
            for wordie in search_ipc.split():
                if wordie not in line.split():
                    found = False
                    break
            # if all(word in line.split() for word in search_ipc.split()):
            if found:
                # Search for the first unknown string
                word = line.split(search_ipc)
                wordList = word[1].split()
                ipc = float(wordList[0])
            found = True
            for wordie in search_accuracy.split():
                if wordie not in line.split():
                    found = False
                    break
            # if all(word in line.split() for word in search_ipc.split()):
            if found:
                # Search for the first unknown string
                word = line.split(search_accuracy)
                wordList = word[1].split()
                accuracy = float(wordList[0])
            found = True
            if ipc is not None and accuracy is not None:
                return (ipc,accuracy)

policies = ['bimodal','hashed_perceptron','gshare','ltage']
plotdata = []


policywiseData = []
for policy in policies:
    os.system('./build_champsim.sh '+ policy +' no no no no lru 1')
    os.system('./run_champsim.sh '+ policy +'-no-no-no-no-lru-1core ' + N_warm + " " + N_sim + " " + trace)
    resultfile = trace + "-"+policy +'-no-no-no-no-lru-1core.txt'
    policywiseData.append(find_Accuracy(resultfile, N_sim))
    #plotdata.append(policywiseData)

#plotting, we generate two different plots for ipc and LLC miss rate
N = 1
ind = np.arange(N)
# ind = [0,1,2,3]
width = 0.05
#ipc_val = [policywiseData[0][0],policywiseData[1][0],policywiseData[2][0],policywiseData[3][0],policywiseData[5][0]]
plt.subplot(1,2,1)
# plt.plot(ind,policywiseData)
bar1 = plt.bar(ind, policywiseData[0][0], width, color = 'r')
bar2 = plt.bar(ind+width, policywiseData[1][0], width, color = 'b')
bar3 = plt.bar(ind+2*width, policywiseData[2][0], width, color = 'g')
#bar4 = plt.bar(ind+3*width, policywiseData[3][0], width, color = 'c')
bar5 = plt.bar(ind+3*width, policywiseData[3][0], width, color = '#FFA500')

plt.xlabel("Different branch predictors")
plt.ylabel("IPC values")
plt.legend( (bar1,bar2,bar3,bar5), ('bimodal','hashed_perceptron','gshare','ltage') )
plt.title("Comparing IPC for different Branch Predictors", fontsize=10)

plt.subplot(1,2,2)
bar1 = plt.bar(ind, policywiseData[0][1], width, color = 'r')
bar2 = plt.bar(ind+width, policywiseData[1][1], width, color = 'b')
bar3 = plt.bar(ind+2*width, policywiseData[2][1], width, color = 'g')
#bar4 = plt.bar(ind+3*width, policywiseData[3][1], width, color = 'c')
bar5 = plt.bar(ind+3*width, policywiseData[3][1], width, color = '#FFA500')

plt.xlabel("Different branch predictors")
plt.ylabel("MPKI")
plt.legend( (bar1,bar2,bar3,bar5), ('bimodal','hashed_perceptron','gshare','ltage') )
plt.title("Comparing misses per 1K instructions for different Branch Predictors", fontsize=10)
plt.suptitle("Plot for trace " + trace,fontsize=20, fontweight='bold')
plt.show()
