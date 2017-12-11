# Speed Up <i>All Pairs Similarity Search (APSS)</i>

## Introduction

 Given a large collection of sparse vector data in a high dimensional space, the All pairs similarity search (APSS) or self-similarity join is the problem of finding all pairs of records that have a similarity score above a given threshold. Similarity between two records is defined via some similarity measure, such as the cosine similarity or the Tanimoto coefficient. APSS is a compute-intensive problem. 

## Problem Statement

The problem tackled in this project is called Min-𝜖 Cosine 𝑘-Nearest Neighbor Graph Construction, and is defined as follows.
Given a set of objects 𝐷, for each object 𝑑i in 𝐷, find the 𝑘 most similar other objects 𝑑j with cosine similarity cos(𝑑i,𝑑j) of at least 𝜖. In this project, the problem must be solved exactly, i.e., all correct neighbors must be reported, along with their correct cosine similarities.

A baseline method, using IdxJoin, has been provided by Prof. David C. Anastasiu, implemented in C. The efficiency of  the solution implemented as part of this project must be compared against IdxJoin on a variety of provided inputs (𝜖, 𝑘, datasets).

## Approach and Methodology

The algorithm implemented as part of this project is the Basic Inverted Index-Based Approach  proposed by the authors in their research.  It is named All-Pairs-0 and appears in Figure 1. in the paper.

## Algorithm Design and Implementation

The implemented algorithm (InvertedIdx) is in file findsim/invertedidx.cpp and has been set as the default mode of execution. All other files in findsim have been given by Prof. David C. Anastasiu. The command to execute the findsim program in invertedidx mode is

  <p align="center">../build/findsim -m iidx -eps 0.3 -k 10 wiki1.csr wiki1.iidx.0.3.10.csr <br>Or<br>../build/findsim -eps 0.3 -k 10 wiki1.csr wiki1.iidx.0.3.10.csr <br>Or<br>../build/findsim wiki1.csr wiki1.iidx.0.3.10.csr

General usage and options of findsim command can be found in [here](https://github.com/sowmyagowri/Speed-up-APSS/blob/master/findsim/README)

Input CSR matrix with empty columns removed V
Vector of inverted indices I1, I2…, Im 
Vector of maps of similarity scores for each document and its corresponding index M
Vector of similarity scores for every input document S
Vector of candidate pairs C
Given similarity threshold value t
Given required no. of neighbors k
Vector of neighbors N

![picture1](https://user-images.githubusercontent.com/25673997/33811183-68ee3f9c-ddc4-11e7-9394-30ed74557416.png)

A python script has been written to execute both the algorithms for all the below mentioned combinations and is available in [findsim/scripts/test_script.py](https://github.com/sowmyagowri/Speed-up-APSS/blob/master/findsim/script/test_script.py). This script also runs the findsim command in eq mode and displays the differences, if any, between the two output files.

* 𝜖 ∈ {0.3,0.4,0.5,0.7,0.9}
* 𝑘 ∈ {10,50,100} and 
* datasets ∈ {wiki1.csr, wiki2.csr}

## Performance Comparison using Graphs
<p align="center">Input File: Wiki1.csr with 10,000 documents</p>
![invertedindex-page-001](https://user-images.githubusercontent.com/25673997/33811863-b9196462-ddcc-11e7-9f15-3d2776a0b542.jpg)


<p align="center">Input File: Wiki2.csr with 50,000 documents</p>

## References

1.	[R. J. Bayardo, Y. Ma, and R. Srikant, “Scaling up all pairs similarity search,” in Proceedings of the 16th International Conference     on World Wide Web, ser. WWW ’07. New York, NY, USA: ACM, 2007, pp. 131–140.](https://static.googleusercontent.com/media/research.google.com/en//pubs/archive/32781.pdf)
2.	Classroom slides on k-nearest neighbor (knn) and IdxJoin Algorithms by [Prof. David C. Anastasiu](http://davidanastasiu.net/) of CMPE department at [San Jose State University (SJSU)](http://www.sjsu.edu/).
