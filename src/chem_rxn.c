/*
 * The AcCoRD Simulator
 * (Actor-based Communication via Reaction-Diffusion)
 *
 * Copyright 2016 Adam Noel. All rights reserved.
 * 
 * For license details, read LICENSE.txt in the root AcCoRD directory
 * For user documentation, read README.txt in the root AcCoRD directory
 *
 * chem_rxn.c - structure for storing chemical reaction properties
 * Last revised for AcCoRD v0.4
 *
 * Revision history:
 *
 * Revision v0.4
 * - modified use of unsigned long and unsigned long long to uint32_t and uint64_t
 * - modified propensity updates to do a full re-calculation in order to avoid
 * numerical underflow problems
 * - removed deprecated debug function
 * - added restriction of chemical reactions to specific regions
 * - renamed region structure and array of structures to accommodate naming
 * of new boundary structure
 *
 * Revision v0.3.1
 * - header added
*/

#include "chem_rxn.h" // for "Public" declarations

#include <stdio.h>
#include <stdlib.h> // for exit(), malloc
#include <stdbool.h> // for C++ bool conventions
#include <string.h> // for strlen(), strcmp()


//
// "Private" Declarations
//

//
// Definitions
//

/* Allocate space for the regionArray members needed for chemical
 * reactions and initialize their values based on the chem_rxn
 * array of chem_rxn_struct
*/
void initialize_region_chem_rxn3D(const short NUM_REGIONS,
	struct region regionArray[],
	const unsigned short NUM_MOL_TYPES,
	const unsigned short MAX_RXNS,
	const struct chem_rxn_struct chem_rxn[])
{
	short i; // Current region
	unsigned short j; // Current reaction
	unsigned short curRxn; // Current reaction in chem_rxn array (when we iterate over region reactions)
	unsigned short k; // Current molecule type OR reaction exception
	unsigned short curProd; // Index of current product (if a reaction has multiple
							// products of the same molecule type)
	uint32_t num_reactants; // Number of reactants in a reaction
	
	bool bFoundReactant; // Have we found the first reactant yet?
	
	bool (* bRxnInRegion)[NUM_REGIONS]; // Which reactions can occur in which regions?
	unsigned short (* rxnInRegionID)[NUM_REGIONS]; // IDs of reactions that can occur in each region
	
	// Build arrays to indicate where each reaction can take place
	bRxnInRegion = malloc(MAX_RXNS * sizeof(bool [NUM_REGIONS]));
	rxnInRegionID = malloc(MAX_RXNS * sizeof(unsigned short [NUM_REGIONS]));
	if(bRxnInRegion == NULL || rxnInRegionID == NULL)
	{
		puts("Memory could not be allocated to region reaction matrix");
		exit(3);
	}
	for(i = 0; i < NUM_REGIONS; i++)
	{
		regionArray[i].numChemRxn = 0;
		for(j = 0; j < MAX_RXNS; j++)
		{
			if(chem_rxn[j].bEverywhere)
				bRxnInRegion[j][i] = true;
			else
				bRxnInRegion[j][i] = false;
			if(regionArray[i].spec.label && strlen(regionArray[i].spec.label) > 0)
			{ // Need to check for reaction exceptions since region has a label
				for(k = 0; k < chem_rxn[j].numRegionExceptions; k++)
				{
					if(chem_rxn[j].regionExceptionLabel[k] &&
						strlen(chem_rxn[j].regionExceptionLabel[k]) > 0 &&
						!strcmp(regionArray[i].spec.label, chem_rxn[j].regionExceptionLabel[k]))
					{ // This region is listed as an exception
						bRxnInRegion[j][i] = !bRxnInRegion[j][i];
					}
				}
			}
			// Count the reaction if it can take place in region
			if(bRxnInRegion[j][i])
			{
				rxnInRegionID[regionArray[i].numChemRxn++][i] = j;
			}
		}
	}
		
	// Allocate memory
	for(i = 0; i < NUM_REGIONS; i++)
	{	// TODO: Separate members that are only needed for meso or micro regions

		regionArray[i].numZerothRxn = 0;
		regionArray[i].numFirstRxn = 0;
		regionArray[i].numSecondRxn = 0;
		
		if(regionArray[i].numChemRxn == 0)
			continue; // No reactions in this region; no need to allocate memory
				
		regionArray[i].numMolChange =
			malloc(regionArray[i].numChemRxn * sizeof(uint64_t *));
		regionArray[i].bMolAdd =
			malloc(regionArray[i].numChemRxn * sizeof(bool *));
		regionArray[i].numRxnProducts =
			malloc(regionArray[i].numChemRxn*sizeof(uint32_t));
		regionArray[i].productID =
			malloc(regionArray[i].numChemRxn * sizeof(unsigned short *));
		regionArray[i].bUpdateProp =
			malloc(regionArray[i].numChemRxn * sizeof(bool *));
		regionArray[i].rxnOrder =
			malloc(regionArray[i].numChemRxn*sizeof(uint32_t));
		regionArray[i].rxnRate =
			malloc(regionArray[i].numChemRxn*sizeof(double));
		regionArray[i].zerothRxn =
			malloc(regionArray[i].numChemRxn*sizeof(unsigned short));
		regionArray[i].firstRxn =
			malloc(regionArray[i].numChemRxn*sizeof(unsigned short));
		regionArray[i].secondRxn =
			malloc(regionArray[i].numChemRxn*sizeof(unsigned short));
		regionArray[i].tZeroth =
			malloc(regionArray[i].numChemRxn*sizeof(double));
		regionArray[i].rxnRateZerothMicro =
			malloc(regionArray[i].numChemRxn*sizeof(double));
		regionArray[i].uniReactant =
			malloc(regionArray[i].numChemRxn*sizeof(unsigned short));
		regionArray[i].numFirstCurReactant =
			malloc(NUM_MOL_TYPES*sizeof(unsigned short));
		regionArray[i].firstRxnID =
			malloc(NUM_MOL_TYPES*sizeof(unsigned short *));
		regionArray[i].uniSumRate =
			malloc(NUM_MOL_TYPES*sizeof(double));
		regionArray[i].uniCumProb =
			malloc(NUM_MOL_TYPES*sizeof(double *));
		regionArray[i].uniRelativeRate =
			malloc(NUM_MOL_TYPES*sizeof(double *));
		regionArray[i].minRxnTimeRV =
			malloc(NUM_MOL_TYPES*sizeof(double));
		regionArray[i].biReactants =
			malloc(regionArray[i].numChemRxn*sizeof(unsigned short [2]));
		
		if(regionArray[i].numMolChange == NULL
			|| regionArray[i].bMolAdd == NULL
			|| regionArray[i].numRxnProducts == NULL
			|| regionArray[i].productID == NULL
			|| regionArray[i].bUpdateProp == NULL
			|| regionArray[i].rxnOrder == NULL
			|| regionArray[i].rxnRate == NULL
			|| regionArray[i].zerothRxn == NULL
			|| regionArray[i].firstRxn == NULL
			|| regionArray[i].secondRxn == NULL
			|| regionArray[i].tZeroth == NULL
			|| regionArray[i].rxnRateZerothMicro == NULL
			|| regionArray[i].uniReactant == NULL
			|| regionArray[i].numFirstCurReactant == NULL
			|| regionArray[i].firstRxnID == NULL
			|| regionArray[i].uniSumRate == NULL
			|| regionArray[i].uniCumProb == NULL
			|| regionArray[i].uniRelativeRate == NULL
			|| regionArray[i].minRxnTimeRV == NULL
			|| regionArray[i].biReactants == NULL)
		{
			puts("Memory could not be allocated for chemical reactions");
			exit(3);
		}
		
		for(j = 0; j < regionArray[i].numChemRxn; j++)
		{
			regionArray[i].numMolChange[j] =
				malloc(NUM_MOL_TYPES * sizeof(uint64_t));
			regionArray[i].bMolAdd[j] =
				malloc(NUM_MOL_TYPES * sizeof(bool));
			regionArray[i].productID[j] =
				malloc(MAX_RXN_PRODUCTS * sizeof(unsigned short));
			regionArray[i].bUpdateProp[j] =
				malloc(NUM_MOL_TYPES * sizeof(bool));
			if(regionArray[i].numMolChange[j] == NULL
				|| regionArray[i].bMolAdd[j] == NULL
				|| regionArray[i].productID[j] == NULL
				|| regionArray[i].bUpdateProp[j] == NULL)
			{
				puts("Memory could not be allocated for chemical reactions");
				exit(3);
			}
		}
		
		for(j = 0; j < NUM_MOL_TYPES; j++)
		{
			regionArray[i].firstRxnID[j] =
				malloc(regionArray[i].numChemRxn * sizeof(unsigned short));
			regionArray[i].uniCumProb[j] =
				malloc(regionArray[i].numChemRxn * sizeof(double));
			regionArray[i].uniRelativeRate[j] =
				malloc(regionArray[i].numChemRxn * sizeof(double));
			if(regionArray[i].firstRxnID[j] == NULL
				|| regionArray[i].uniCumProb[j] == NULL
				|| regionArray[i].uniRelativeRate[j] == NULL)
			{
				puts("Memory could not be allocated for chemical reactions");
				exit(3);
			}
		}
	}
	
	// Initialize the allocated elements based on chem_rxn array
	for(i = 0; i < NUM_REGIONS; i++)
	{	
		if(regionArray[i].numChemRxn == 0)
			continue; // No reactions in this region; no need to allocate memory
		
		// Initialize elements that are indexed by reaction
		for(j = 0; j < regionArray[i].numChemRxn; j++)
		{
			num_reactants = 0;
			bFoundReactant = false;
			regionArray[i].numRxnProducts[j] = 0;
			curRxn = rxnInRegionID[j][i]; // Current reaction in chem_rxn array
			
			for(k = 0; k < NUM_MOL_TYPES; k++)
			{
				num_reactants += chem_rxn[curRxn].reactants[k];
				
				
				switch(chem_rxn[curRxn].reactants[k])
				{
					case 1:
						regionArray[i].uniReactant[j] = k;
						if(bFoundReactant)
							regionArray[i].biReactants[j][1] = k;
						else
							regionArray[i].biReactants[j][0] = k;
						bFoundReactant = true;
						break;
					case 2:
						regionArray[i].biReactants[j][0] = k;
						regionArray[i].biReactants[j][1] = k;
						break;
				}
				
				// Is there a net gain or loss of molecule k?
				if(chem_rxn[curRxn].reactants[k] < chem_rxn[curRxn].products[k])
				{	// Net gain of molecule type k
					regionArray[i].numMolChange[j][k] =
						chem_rxn[curRxn].products[k] - chem_rxn[curRxn].reactants[k];
					regionArray[i].bMolAdd[j][k] = true;
				} else
				{	// Net loss of molecule type k
					regionArray[i].numMolChange[j][k] =
						chem_rxn[curRxn].reactants[k] - chem_rxn[curRxn].products[k];
					regionArray[i].bMolAdd[j][k] = false;					
				}
				
				// Is molecule k a reactant?
				// If it is, then we must update propensity if number
				// of k molecules changes
				if(chem_rxn[curRxn].reactants[k] > 0)
					regionArray[i].bUpdateProp[j][k] = true;
				else
					regionArray[i].bUpdateProp[j][k] = false;
				
				// Is molecule k a product?
				// If it is, then we must list it in the listing of reaction products
				// for the current reaction
				for(curProd = 0; curProd < chem_rxn[curRxn].products[k]; curProd++)
				{
					regionArray[i].productID[j][regionArray[i].numRxnProducts[j]++] = k;
				}
			}
			
			regionArray[i].rxnOrder[j] = num_reactants;
			
			switch(num_reactants)
			{
				case 0:
					// rxnRate must be calculated for one subvolume
					regionArray[i].rxnRate[j] = chem_rxn[curRxn].k
						*regionArray[i].actualSubSize * regionArray[i].actualSubSize * regionArray[i].actualSubSize;
					// microscopic 0th order rate depends on total region volume
					regionArray[i].rxnRateZerothMicro[regionArray[i].numZerothRxn] =
						chem_rxn[curRxn].k * regionArray[i].volume;
					regionArray[i].zerothRxn[regionArray[i].numZerothRxn++] = j;
					break;
				case 1:
					regionArray[i].rxnRate[j] = chem_rxn[curRxn].k;
					regionArray[i].firstRxn[regionArray[i].numFirstRxn++] = j;
					break;
				case 2:
					regionArray[i].rxnRate[j] = chem_rxn[curRxn].k
						/regionArray[i].actualSubSize / regionArray[i].actualSubSize / regionArray[i].actualSubSize;
					regionArray[i].secondRxn[regionArray[i].numSecondRxn++] = j;
					break;
				default:
					puts("A reaction has too many reactants");
					exit(3);
			}
		}
		
		// Initialize elements that are sorted by reactant
		for(j = 0; j < NUM_MOL_TYPES; j++)
		{
			regionArray[i].numFirstCurReactant[j] = 0;
			regionArray[i].uniSumRate[j] = 0.;
			regionArray[i].uniCumProb[j][0] = 0.;
		
			// Scan reactions to see which have current molecule as a reactant
			for(k = 0; k < regionArray[i].numChemRxn; k++)
			{
				switch(regionArray[i].rxnOrder[k])
				{
					case 1:						
						if(chem_rxn[k].reactants[j] > 0)
						{
							regionArray[i].firstRxnID[j][regionArray[i].numFirstCurReactant[j]] = k;
							regionArray[i].uniSumRate[j] += regionArray[i].rxnRate[k];		
							
							regionArray[i].numFirstCurReactant[j]++;
						}
						break;
					case 2:
						break;
				}
			}
			
			// Scan reactions with current molecule as reactant to determine the
			// cumulative reaction probabilities
			for(k = 0; k < regionArray[i].numFirstCurReactant[j]; k++)
			{
				if(k > 0)
					regionArray[i].uniCumProb[j][k] = regionArray[i].uniCumProb[j][k-1];
				
				// Relative rate will be used when we need to recalculate the probabilities
				// due to smaller time step size
				regionArray[i].uniRelativeRate[j][k] =
					regionArray[i].rxnRate[regionArray[i].firstRxnID[j][k]]
					/ regionArray[i].uniSumRate[j];
				
				// The following cumulative probability calculation is valid for when
				// a molecule existed from the end of the last region time step
				regionArray[i].uniCumProb[j][k] +=
					regionArray[i].uniRelativeRate[j][k]
					* (1 - exp(-regionArray[i].spec.dt*regionArray[i].uniSumRate[j]));
				
			}
			
			regionArray[i].minRxnTimeRV[j] =
				exp(-regionArray[i].spec.dt*regionArray[i].uniSumRate[j]);
		}
		
	}
	
	// Free memory of temporary parameters
	if(bRxnInRegion != NULL)
		free(bRxnInRegion);
	if(rxnInRegionID != NULL)
		free(rxnInRegionID);
}

// Free memory of region parameters
void delete_region_chem_rxn3D(const short NUM_REGIONS,
	const unsigned short NUM_MOL_TYPES,
	struct region regionArray[])
{
	short i; // Current region
	unsigned short j; // Current reaction
	
	if(regionArray == NULL)
		return;
	
	for(i = 0; i < NUM_REGIONS; i++)
	{
		if(regionArray[i].numChemRxn == 0)
			continue; // No reactions in this region; no need to free memory
		
		for(j = 0; j < regionArray[i].numChemRxn; j++)
		{
			if(regionArray[i].numMolChange[j] != NULL)
				free(regionArray[i].numMolChange[j]);
			if(regionArray[i].bMolAdd[j] != NULL)
				free(regionArray[i].bMolAdd[j]);
			if(regionArray[i].productID[j] != NULL)
				free(regionArray[i].productID[j]);
			if(regionArray[i].bUpdateProp[j] != NULL)
				free(regionArray[i].bUpdateProp[j]);
		}
		
		for(j = 0; j < NUM_MOL_TYPES; j++)
		{
			if(regionArray[i].firstRxnID[j] != NULL)
				free(regionArray[i].firstRxnID[j]);
			if(regionArray[i].uniCumProb[j] != NULL)
				free(regionArray[i].uniCumProb[j]);
			if(regionArray[i].uniRelativeRate[j] != NULL)
				free(regionArray[i].uniRelativeRate[j]);
		}
		
		if(regionArray[i].numMolChange != NULL) free(regionArray[i].numMolChange);
		if(regionArray[i].bMolAdd != NULL) free(regionArray[i].bMolAdd);
		if(regionArray[i].numRxnProducts != NULL) free(regionArray[i].numRxnProducts);
		if(regionArray[i].productID != NULL) free(regionArray[i].productID);
		if(regionArray[i].bUpdateProp != NULL) free(regionArray[i].bUpdateProp);
		if(regionArray[i].rxnOrder != NULL) free(regionArray[i].rxnOrder);
		if(regionArray[i].rxnRate != NULL) free(regionArray[i].rxnRate);
		if(regionArray[i].zerothRxn != NULL) free(regionArray[i].zerothRxn);
		if(regionArray[i].firstRxn != NULL) free(regionArray[i].firstRxn);
		if(regionArray[i].secondRxn != NULL) free(regionArray[i].secondRxn);
		if(regionArray[i].tZeroth != NULL) free(regionArray[i].tZeroth);
		if(regionArray[i].rxnRateZerothMicro != NULL) free(regionArray[i].rxnRateZerothMicro);
		if(regionArray[i].uniReactant != NULL) free(regionArray[i].uniReactant);
		if(regionArray[i].numFirstCurReactant != NULL) free(regionArray[i].numFirstCurReactant);
		if(regionArray[i].firstRxnID != NULL) free(regionArray[i].firstRxnID);
		if(regionArray[i].uniSumRate != NULL) free(regionArray[i].uniSumRate);
		if(regionArray[i].uniCumProb != NULL) free(regionArray[i].uniCumProb);
		if(regionArray[i].uniRelativeRate != NULL) free(regionArray[i].uniRelativeRate);
		if(regionArray[i].minRxnTimeRV != NULL) free(regionArray[i].minRxnTimeRV);
		if(regionArray[i].biReactants != NULL) free(regionArray[i].biReactants);
	}
}