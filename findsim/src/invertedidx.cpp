/*!
 \file  invertedidx.cc
 \brief This file contains a faster implementation of the all pair similarities search that uses the input eps during indexing and all its related functions.
 Exploiting the eps during indexing reduces the number of features indexed in the first place.

 This approach is based on the algorithm in Figure 1 in the paper
 R. J. Bayardo, Yiming Ma, Ramakrishnan Srikant. Scaling Up All-Pairs Similarity Search. In Proc. of the 16th Int'l Conf. on World Wide Web, 131-140, 2007

 This implementation dramatically reduces the overhead by
 	 1. scanning the dataset and incrementally building the inverted lists and then
 	 2. scanning the inverted lists to perform similarity score accumulation

\author Sowmya Gowrishankar
*/

#include "includes.h"

#include <vector>       // std::vector
#include <map>			// std::map
#include <set>			// std::set
#include <functional>	// std::greater
#include <iterator>     // std::iterator


using namespace std;

// forward declarations
void findMatches(const int doc_id, const vector<vector<pair<int, float>>>& invertedIndex, const float eps, da_csr_t *docs, idx_t *ncands,
				vector<map<int, float>> *matches);

/**
 * Main entry point to Inverted Index APSS.
 */
void invertedidx(params_t *params)
{
	ssize_t i, j;
	size_t nsims, ncands, nnz;
	idx_t nrows, ncand, progressInd, pct;
	da_csr_t *docs, *neighbors=NULL;

	docs    = params->docs;
	nrows   = docs->nrows;  // num rows
	ncands  = 0; // number of considered candidates (computed similarities)
	nsims   = 0; // number of similar documents found

    /** Pre-process input matrix: remove empty columns, ensure sorted column ids, scale by IDF **/

    /* compact the column space */
    da_csr_CompactColumns(docs);
    if(params->verbosity > 0)
        printf("Docs matrix: " PRNT_IDXTYPE " rows, " PRNT_IDXTYPE " cols, "
            PRNT_PTRTYPE " nnz\n", docs->nrows, docs->ncols, docs->rowptr[docs->nrows]);

    /* sort the column space */
    da_csr_SortIndices(docs, DA_ROW);

    /* scale term values */
    if(params->verbosity > 0)
        printf("   Scaling input matrix.\n");
    da_csr_Scale(docs);


	timer_start(params->timer_3); /* overall knn graph construction time */

    /* normalize docs rows */
    da_csr_Normalize(docs, DA_ROW, 2);

    /* create inverted index - column version of the matrix */
    da_csr_CreateIndex(docs, DA_COL);

    neighbors = da_csr_Create();
    neighbors->nrows = neighbors->ncols = nrows;
    nnz = params->k * docs->nrows; /* max number of neighbors */
    neighbors->rowptr = da_pmalloc(nrows + 1, "simSearchSetup: neighbors->rowptr");
    neighbors->rowind = da_imalloc(nnz, "simSearchSetup: neighbors->rowind");
    neighbors->rowval = da_vmalloc(nnz, "simSearchSetup: neighbors->rowval");
    neighbors->rowptr[0] = 0;

    /* set up progress indicator */
    da_progress_init_steps(pct, progressInd, nrows, 10);
    if(params->verbosity > 0)
    	printf("Progress Indicator: ");

    // Vector of pairs to store the weights within the inverted index itself for each feature/dimension.
    // This vector of pairs is again stored in a vector that is the size of the total number of feature/dimensions.
    // Using vector of maps instead of inner vector of pairs drastically reduced the speedup.
    // Hence, using inner vector of pairs.
    // Vector data format is as follows:
    // V1 in invertedIndex for Feature 1: [(v11, 0.32), (v230, 0.48), ()....]
    // V2 in invertedIndex for Feature 2: [(v20, 0.60), (v100, 0.45), ()....]
    vector<vector<pair<int, float>>> invertedIndex;

    // Resize the vector to the total number of features/dimensions.
    invertedIndex.resize(docs->ncols);

    // Vector of maps to store the similarity of each document with every other document.
    // Using maps of vector id (key)to weight (value) as given in the paper.
    // Using inner vector of pairs instead of maps didn't affect the speedup. Hence, using the implementation mentioned in the paper.
    // First element in the pairs is the computed similarity value and the second element is the document id against which the similarity has been computed
    // This vector of similarity pairs is again stored in a vector that is the size of the total number of documents.
    // The vector is declared here and a reference is sent to the similarities computation function.
    // Vector data format is as follows:
    // V1 in matches: [(v2, 0.32), (v3, 0.48), ()....]
    // V2 in matches: [(v1, 0.23), (v3, 0.68), ()....]
    vector<map<int, float>> matches;

    // Resize the vector to the total number of documents
    matches.resize(docs->nrows);

    // Build the inverted index and scan the inverted index list to perform similarity score accumulation.
    for (i=0; i <= docs->nrows; i++) {
        findMatches(i, invertedIndex, params->epsilon, docs, &ncand, &matches);
        ncands += ncand;
        for (j = docs->rowptr[i]; j < docs->rowptr[i+1]; j++) {
        	invertedIndex[docs->rowind[j]].push_back({i, docs->rowval[j]});
        }

        // Update progress indicator.
        if (params->verbosity > 0 && i % progressInd == 0) {
        	da_progress_advance_steps(pct, 10);
        }
    }

    /* execute search */
    // It has been specifically mentioned that we must write our own base function for finding neighbors that does not call any other 3rd party functions.
    // Hence, not using sort function to sort the inner vector of pairs.
    // Instead using STL Set in which the Pairs are always inserted in the Set based on the first value (key).
    // Since, 'greater' function during declaration the pairs are automatically inserted in non-increasing order of cosine similarity.
    // The top k can then be easily obtained from this sorted set of pairs with a simple 'for' loop and 'if no. of candidates < k' condition
    nsims = 0;

    for (i=0; i < matches.size(); ++i) {

    	// Using the default constructor to initialize the set of pairs for every query vector
    	set<pair<float, int>, greater<pair<float, int>>> candidatePairs;
        for (const auto& similarity : matches[i]) {
        	candidatePairs.insert(pair<float, int>(similarity.second, similarity.first));
        }

        j = 0;
        for (const auto &candidatePair : candidatePairs) {
        	//Strictly smaller as the index starts at 0
        	if (j < params->k){
        		neighbors->rowind[nsims] = candidatePair.second;
				neighbors->rowval[nsims] = candidatePair.first;
				nsims++;
				j++;
            }
        }
        neighbors->rowptr[i+1] = nsims;
    }

    // Print progress indicator
    if (params->verbosity > 0) {
    	pct = 100;
    	da_progress_finalize_steps(pct, 10);
        printf("\n");
    }

	timer_stop(params->timer_3); // find neighbors time

	printf("Number of computed similarities: %zu\n", ncands);
    printf("Number of neighbors: %zu\n", nsims);

    /* Write ouptut */
	if(params->oFile){
	    da_csr_Write(neighbors, params->oFile, DA_FMT_CSR, 1, 1);
	    printf("Wrote output to %s\n", params->oFile);
	}
	da_csr_Free(&neighbors);
}

/**
 * Scans the inverted lists to perform similarity score accumulation.
 * \param doc_id The document index for which the similarities need to be computed
 * \param invertedIndex Vector of inverted indices with vector weights for each feature/index stored within the inverted index itself.
 * \param eps Minimum similarity between query and neighbors
 * \param docs Reference to the entire document stored as a sparse CSR matrix
 * \param matches Reference to Vector of pairs to store the similarity of each document with every other document.
 * \param ncands Reference to int variable to hold number of candidates
 *
 * \return Number of similar pairs found
 */
void findMatches(const int doc_id, const vector<vector<pair<int, float>>>& invertedIndex, const float eps, da_csr_t* docs, idx_t *ncands,
				vector<map<int, float>>* matches){

	ssize_t i;
	idx_t ncand;

    vector<float> cosineSimilarities;
    cosineSimilarities.resize(doc_id, 0);

    ncand = 0;

    for(i=docs->rowptr[doc_id]; i < docs->rowptr[doc_id+1]; i++) {
    	for (const auto& candidatePair: invertedIndex[docs->rowind[i]]) {
    		cosineSimilarities[candidatePair.first] += docs->rowval[i] * candidatePair.second;
    	}
    }

    // Accumulate the cosine similarity greater than the input eps value
    for(i=0; i < cosineSimilarities.size(); i++) {
    	if (cosineSimilarities[i] >= eps) {
				(*matches)[i].insert(pair<int, float>(doc_id, cosineSimilarities[i]));
    			(*matches)[doc_id].insert(pair<int, float>(i, cosineSimilarities[i]));
    			ncand++;
    	}
    }

    *ncands = ncand; /* number of candidates/computed similarities for this query object */
}
