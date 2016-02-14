/*
 * The AcCoRD Simulator
 * (Actor-based Communication via Reaction-Diffusion)
 *
 * Copyright 2016 Adam Noel. All rights reserved.
 * 
 * For license details, read LICENSE.txt in the root AcCoRD directory
 * For user documentation, read README.txt in the root AcCoRD directory
 *
 * file_io.c - interface with JSON configuration files
 * Last revised for AcCoRD v0.4
 *
 * Revision history:
 *
 * Revision v0.4
 * - modified use of unsigned long and unsigned long long to uint32_t and uint64_t
 * - added options to accommodate spherical regions and actors
 * - added restriction of chemical reactions to specific regions
 * - renamed region structure and array of structures to accommodate naming
 * of new boundary structure
 *
 * Revision v0.3.1.1
 * - corrected warning message when loading a configuration file to display the correct
 *   keys to quit or continue
 *
 * Revision v0.3.1
 * - check all JSON members for existence and correct format before reading them in
 * - header added
*/

#include "file_io.h"

// Load configuration file
void loadConfig3D(const char * CONFIG_NAME,
	uint32_t customSEED,
	struct simSpec3D * curSpec)
{
	FILE * configFile;
	long fileLength;
	cJSON * configJSON;
	cJSON *simControl, *environment, *regionSpec,
		*curObj, *curObjInner,
		*actorSpec, *actorShape, *actorModScheme,
		*diffCoef,
		*chemSpec, *rxnSpec;
	int arrayLen;
	char * tempString;
	int curArrayItem;
	unsigned short curMolType, i;
	char * configContent;
	char * configDir = "../config/";
	char * configNameFull;
	unsigned int dirLength, nameLength;
	bool bWarn = false; // Was there at least one warning?
	bool bWarnOverride; // Did the user request to override pause for warnings?
	int numWarn = 0; // Number of warnings found in configuration file.
	int ch;
	size_t temp; // Garbage variable for discarded file content length
	
	// Construct full name of configuration file
	dirLength = strlen(configDir);
	nameLength = strlen(CONFIG_NAME);
	configNameFull = malloc(dirLength + nameLength + 1);
	if(configNameFull == NULL)
	{
		puts("Memory could not be allocated to build the configuration file name");
		exit(3);
	}
	configNameFull[0] = '\0'; // Initialize full name to the empty string
	strcat(configNameFull,configDir);
	strcat(configNameFull,CONFIG_NAME);
	
	// Open configuration file
	configFile = fopen(configNameFull, "r");
	if(configFile == NULL)
	{
		fprintf(stderr,"Cannot open configuration file %s.\n",CONFIG_NAME);
		exit(3);
	}
	
	// Read in contents of configuration file
	fseek(configFile, 0, SEEK_END);
	fileLength = ftell(configFile);
	fseek(configFile,0,SEEK_SET);
	
	configContent = malloc(fileLength + 1);
	if(configContent == NULL)
	{
		puts("Memory could not be allocated to load the configuration file");
		exit(3);
	}
	temp = fread(configContent,1,fileLength,configFile);
	fclose(configFile);
	
	// Convert file contents into a JSON object
	configJSON = cJSON_Parse(configContent);
	if (!configJSON)
	{
		printf("Error in format of configuration file in area of: [%s]\n",cJSON_GetErrorPtr());
		exit(3);
	}
	
	// Check Existence of Primary Structure Objects
	if(!cJSON_bItemValid(configJSON,"Simulation Control", cJSON_Object))
	{
		puts("ERROR: Configuration file is missing \"Simulation Control\" object.");
		exit(3);
	}
	if(!cJSON_bItemValid(configJSON,"Environment", cJSON_Object))
	{
		puts("ERROR: Configuration file is missing \"Environment\" object.");
		exit(3);
	}
	if(!cJSON_bItemValid(configJSON,"Chemical Properties", cJSON_Object))
	{
		puts("ERROR: Configuration file is missing \"Chemical Properties\" object.");
		exit(3);
	}
	
	// Check for warning override
	if(!cJSON_bItemValid(configJSON,"Warning Override", cJSON_True))
	{
		printf("WARNING %d: Configuration file is missing \"Warning Override\" boolean. Simulation will require user confirmation to execute.\n", numWarn++);
		bWarnOverride = false;
		bWarn = true;
	} else
	{
		bWarnOverride = cJSON_GetObjectItem(configJSON,"Warning Override")->valueint;
		if(bWarnOverride)
		{
			puts("NOTE: Warning override enabled. Simulation will execute automatically, even if warnings appear in the configuration file.");
		} else
		{
			puts("NOTE: Warning override disabled. Simulation will require user confirmation to execute if warnings appear in the configuration file.");
		}
	}
	
	//
	// Transfer JSON content to Simulation Structure	
	//
	
	// Load Simulation Control Object
	simControl = cJSON_GetObjectItem(configJSON,"Simulation Control");
	if(customSEED > 0)
	{ // User specified a seed when simulation was called
		curSpec->SEED = customSEED;
	} else
	{ // Use seed listed in the configuration, if it exists
		if(cJSON_bItemValid(simControl,"Random Number Seed", cJSON_Number))
		{
			curSpec->SEED =
				cJSON_GetObjectItem(simControl,"Random Number Seed")->valueint;
		} else
		{
			bWarn = true;
			printf("WARNING %d: \"Random Number Seed\" not defined and no custom seed specified. Assigning default value of \"0\".\n", numWarn++);
			curSpec->SEED = 0;
		}		
	}
	
	if(!cJSON_bItemValid(configJSON,"Output Filename", cJSON_String) ||
		strlen(cJSON_GetObjectItem(configJSON,"Output Filename")->valuestring) < 1)
	{ // Config file does not list a valid Output Filename
		bWarn = true;
		printf("WARNING %d: \"Output Filename\" not defined or has length zero. Assigning default value of \"test\".\n", numWarn++);
		curSpec->OUTPUT_NAME = stringAllocate(strlen("test") + 15);
		sprintf(curSpec->OUTPUT_NAME, "%s_SEED%d.txt", "test", curSpec->SEED);
	} else{
		curSpec->OUTPUT_NAME = stringAllocate(strlen(cJSON_GetObjectItem(configJSON,
			"Output Filename")->valuestring) + 15);
		sprintf(curSpec->OUTPUT_NAME, "%s_SEED%d.txt", cJSON_GetObjectItem(configJSON,
			"Output Filename")->valuestring, curSpec->SEED);
	}
	
	if(!cJSON_bItemValid(simControl,"Number of Repeats", cJSON_Number) ||
		cJSON_GetObjectItem(simControl,"Number of Repeats")->valueint < 0)
	{ // Config file does not list a valid Number of Repeats
		bWarn = true;
		printf("WARNING %d: \"Number of Repeats\" not defined or has invalid value. Assigning default value of \"1\" realization.\n", numWarn++);
		curSpec->NUM_REPEAT = 1;
	} else{
		curSpec->NUM_REPEAT =
			cJSON_GetObjectItem(simControl,"Number of Repeats")->valueint;
	}
		
	if(!cJSON_bItemValid(simControl,"Final Simulation Time", cJSON_Number) ||
		cJSON_GetObjectItem(simControl,"Final Simulation Time")->valuedouble < 0)
	{ // Config file does not list a valid Final Simulation Time
		bWarn = true;
		printf("WARNING %d: \"Final Simulation Time\" not defined or has invalid value. Assigning default value of \"0\" seconds.\n", numWarn++);
		curSpec->TIME_FINAL = 0.;
	} else{
		curSpec->TIME_FINAL =
			cJSON_GetObjectItem(simControl,"Final Simulation Time")->valuedouble;
	}
		
	if(!cJSON_bItemValid(simControl,"Global Microscopic Time Step", cJSON_Number) ||
		cJSON_GetObjectItem(simControl,"Global Microscopic Time Step")->valuedouble < 0)
	{ // Config file does not list a valid Global Microscopic Time Step
		bWarn = true;
		printf("WARNING %d: \"Global Microscopic Time Step\" not defined or has invalid value. Assigning default value of \"0\" seconds.\n", numWarn++);
		curSpec->DT_MICRO = 0.;
	} else{
		curSpec->DT_MICRO =
			cJSON_GetObjectItem(simControl,"Global Microscopic Time Step")->valuedouble;
	}
	
	if(!cJSON_bItemValid(simControl,"Max Number of Progress Updates", cJSON_Number) ||
		cJSON_GetObjectItem(simControl,"Max Number of Progress Updates")->valueint < 0)
	{ // Config file does not list a valid Number of Repeats
		bWarn = true;
		printf("WARNING %d: \"Max Number of Progress Updates\" not defined or has invalid value. Assigning default value of \"10\" updates.\n", numWarn++);
		curSpec->MAX_UPDATES = 10;
	} else{
		curSpec->MAX_UPDATES =
			cJSON_GetObjectItem(simControl,"Max Number of Progress Updates")->valueint;
	}
	
	// Load Chemical Properties Object
	chemSpec = cJSON_GetObjectItem(configJSON,"Chemical Properties");
		
	if(!cJSON_bItemValid(chemSpec,"Number of Molecule Types", cJSON_Number) ||
		cJSON_GetObjectItem(chemSpec,"Number of Molecule Types")->valueint < 1 ||
		cJSON_GetObjectItem(chemSpec,"Number of Molecule Types")->valueint > MAX_MOL_TYPES)
	{ // Config file does not list a valid Number of Molecule Types
		bWarn = true;
		printf("WARNING %d: \"Number of Molecule Types\" not defined or has invalid value. Assigning default value of \"1\" type.\n", numWarn++);
		curSpec->NUM_MOL_TYPES = 1;
	} else{
		curSpec->NUM_MOL_TYPES =
			cJSON_GetObjectItem(chemSpec,"Number of Molecule Types")->valueint;
	}
	
	curSpec->DIFF_COEF = malloc(curSpec->NUM_MOL_TYPES * sizeof(double));
	if(curSpec->DIFF_COEF == NULL)
	{
		puts("Memory could not be allocated to load diffusion coefficient");
		exit(3);
	}
	
	if(!cJSON_bItemValid(chemSpec,"Diffusion Coefficients", cJSON_Array) ||
		cJSON_GetArraySize(cJSON_GetObjectItem(chemSpec,"Diffusion Coefficients")) != curSpec->NUM_MOL_TYPES)
	{ // Config file does not list a valid Diffusion Coefficients array
		bWarn = true;
		printf("WARNING %d: \"Diffusion Coefficients\" not defined or not of correct length. Assigning default value of \"0\" to each molecule type.\n", numWarn++);
		for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
		{
			curSpec->DIFF_COEF[curMolType] = 0.;
		}
	} else{
		diffCoef = cJSON_GetObjectItem(chemSpec, "Diffusion Coefficients");
		for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES;
			curMolType++)
		{
			if(!cJSON_bArrayItemValid(diffCoef,curMolType, cJSON_Number) ||
				cJSON_GetArrayItem(diffCoef,curMolType)->valuedouble < 0)
			{
				bWarn = true;
				printf("WARNING %d: \"Diffusion Coefficients\" item %d not defined or has an invalid value. Assigning default value of \"0\".\n", numWarn++, curMolType);
				curSpec->DIFF_COEF[curMolType] = 0;
			} else
			{
				curSpec->DIFF_COEF[curMolType] =
					cJSON_GetArrayItem(diffCoef, curMolType)->valuedouble;
			}
		}
	}
	
	if(!cJSON_bItemValid(chemSpec,"Chemical Reaction Specification", cJSON_Array))
	{
		bWarn = true;
		printf("WARNING %d: Configuration file is missing \"Chemical Reaction Specification\" array. Assuming that no chemcial reactions are possible.", numWarn++);
		curSpec->MAX_RXNS = 0;
	} else
	{
		rxnSpec = cJSON_GetObjectItem(chemSpec, "Chemical Reaction Specification");
		curSpec->MAX_RXNS =
			cJSON_GetArraySize(rxnSpec);
		curSpec->chem_rxn =
			malloc(curSpec->MAX_RXNS *
			sizeof(struct chem_rxn_struct));
		if(curSpec->chem_rxn == NULL)
		{
			puts("Memory could not be allocated to load configuration chemical reaction details");
			exit(3);
		}
		for(curArrayItem = 0;
			curArrayItem < curSpec->MAX_RXNS; curArrayItem++)
		{
			if(!cJSON_bArrayItemValid(rxnSpec, curArrayItem, cJSON_Object))
			{
				bWarn = true;
				printf("WARNING %d: \"Chemical Reaction Specification\" item %d is not a JSON object. Creating empty reaction.\n", numWarn++, curArrayItem);
				curSpec->chem_rxn[curArrayItem].k = 0.;
				for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES;
				curMolType++)
				{
					curSpec->chem_rxn[curArrayItem].reactants[curMolType] = 0;
					curSpec->chem_rxn[curArrayItem].products[curMolType] = 0;
				}
			} else
			{			
				curObj = cJSON_GetArrayItem(rxnSpec, curArrayItem);
				if(!cJSON_bItemValid(curObj,"Reaction Rate", cJSON_Number) ||
					!cJSON_bItemValid(curObj,"Reactants", cJSON_Array) ||
					!cJSON_bItemValid(curObj,"Products", cJSON_Array) ||
					cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Reactants")) != curSpec->NUM_MOL_TYPES ||
					cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Products")) != curSpec->NUM_MOL_TYPES ||
					cJSON_GetObjectItem(curObj,"Reaction Rate")->valuedouble < 0.)
				{
					bWarn = true;
					printf("WARNING %d: \"Chemical Reaction Specification\" item %d has missing parameters, an invalid reaction rate, or an incorrect number of molecule types. Creating empty reaction.\n", numWarn++, curArrayItem);
					curSpec->chem_rxn[curArrayItem].k = 0.;
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES;
					curMolType++)
					{
						curSpec->chem_rxn[curArrayItem].reactants[curMolType] = 0;
						curSpec->chem_rxn[curArrayItem].products[curMolType] = 0;
					}
				} else
				{
					if(!cJSON_bItemValid(curObj,"Default Everywhere?", cJSON_True))
					{ // Region does not have a valid Default Everywhere?
						bWarn = true;
						printf("WARNING %d: Chemical reaction %d does not have a valid \"Default Everywhere?\". Assigning default value \"true\".\n", numWarn++, curArrayItem);
						curSpec->chem_rxn[curArrayItem].bEverywhere = true;
					} else
					{
						curSpec->chem_rxn[curArrayItem].bEverywhere = 
							cJSON_GetObjectItem(curObj, "Default Everywhere?")->valueint;
					}
						
					// Record exceptions to the default reaction location
					if(!cJSON_bItemValid(curObj,"Exception Regions", cJSON_Array))
					{ // Chemical reaction does not have an Exception Regions array
						bWarn = true;
						printf("WARNING %d: Chemical reaction %d has a missing or invalid \"Default Everywhere?\". Assigning default value of \"0\" exceptions.\n", numWarn++, curArrayItem);
						curSpec->chem_rxn[curArrayItem].numRegionExceptions = 0;
					} else
					{
						// Read number of exceptions
						curSpec->chem_rxn[curArrayItem].numRegionExceptions =
							cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Exception Regions"));
						
						curSpec->chem_rxn[curArrayItem].regionExceptionLabel =
							malloc(curSpec->chem_rxn[curArrayItem].numRegionExceptions * sizeof(char *));
						if(curSpec->chem_rxn[curArrayItem].regionExceptionLabel == NULL)
						{
							puts("Memory could not be allocated to load chemical reaction region exceptions");
							exit(3);
						}
						
						// Read in names of exception regions							
						curObjInner = cJSON_GetObjectItem(curObj,"Exception Regions");
						for(i = 0; i < curSpec->chem_rxn[curArrayItem].numRegionExceptions; i++)
						{
							if(!cJSON_bArrayItemValid(curObjInner,i, cJSON_String))
							{ // Exception region is not a valid string. Ignore
								bWarn = true;
								printf("WARNING %d: Chemical reaction %d exception region %d is not a valid string. Assigning empty string.\n", numWarn++, curArrayItem, i);
								curSpec->chem_rxn[curArrayItem].regionExceptionLabel[i] = '\0';
							} else
							{
								curSpec->chem_rxn[curArrayItem].regionExceptionLabel[i] =
									stringAllocate(strlen(cJSON_GetArrayItem(curObjInner,i)->valuestring));
								sprintf(curSpec->chem_rxn[curArrayItem].regionExceptionLabel[i],
									"%s", cJSON_GetArrayItem(curObjInner,i)->valuestring);
							}					
						}
					}
					
					curSpec->chem_rxn[curArrayItem].k = 
						cJSON_GetObjectItem(curObj, "Reaction Rate")->valuedouble;
					curObjInner = cJSON_GetObjectItem(curObj,"Reactants");
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES;
					curMolType++)
					{
						if(!cJSON_bArrayItemValid(curObjInner,curMolType, cJSON_Number) ||
							cJSON_GetArrayItem(curObjInner,curMolType)->valueint < 0)
						{
							bWarn = true;
							printf("WARNING %d: Molecule type %d has an incorrect number of reactants in reaction %d. Setting to default value of \"0\".\n", numWarn++, curMolType, curArrayItem);
							curSpec->chem_rxn[curArrayItem].reactants[curMolType] = 0;
						} else
						{
							curSpec->chem_rxn[curArrayItem].reactants[curMolType] =
								cJSON_GetArrayItem(curObjInner, curMolType)->valueint;
						}
					}
					
					curObjInner = cJSON_GetObjectItem(curObj,"Products");
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES;
					curMolType++)
					{
						if(!cJSON_bArrayItemValid(curObjInner,curMolType, cJSON_Number) ||
							cJSON_GetArrayItem(curObjInner,curMolType)->valueint < 0)
						{
							bWarn = true;
							printf("WARNING %d: Molecule type %d has an incorrect number of products in reaction %d. Setting to default value of \"0\".\n", numWarn++, curMolType, curArrayItem);
							curSpec->chem_rxn[curArrayItem].products[curMolType] = 0;
						} else
						{
							curSpec->chem_rxn[curArrayItem].products[curMolType] =
								cJSON_GetArrayItem(curObjInner, curMolType)->valueint;
						}
					}
				}
			}
		}
	}
	
	// Load Environment Object
	environment = cJSON_GetObjectItem(configJSON,"Environment");
	if(!cJSON_bItemValid(environment,"Region Specification", cJSON_Array) ||
		cJSON_GetArraySize(cJSON_GetObjectItem(environment,"Region Specification")) < 1)
	{
		puts("ERROR: Configuration file is missing \"Region Specification\" array in \"Environment\" object or it has a length less than 1.");
		exit(3);
	}
	if(!cJSON_bItemValid(environment,"Actor Specification", cJSON_Array) ||
		cJSON_GetArraySize(cJSON_GetObjectItem(environment,"Actor Specification")) < 1)
	{
		puts("ERROR: Configuration file is missing \"Actor Specification\" array in \"Environment\" object or it has a length less than 1.");
		exit(3);
	}
	
	if(!cJSON_bItemValid(environment,"Number of Dimensions", cJSON_Number) ||
		cJSON_GetObjectItem(environment,"Number of Dimensions")->valueint != 3)
	{
		bWarn = true;
		printf("WARNING %d: \"Number of Dimensions\" not defined or not equal to 3. Setting to default value of \"3\".\n", numWarn++);
		curSpec->NUM_DIM = 3;
	} else
	{
		curSpec->NUM_DIM =
			cJSON_GetObjectItem(environment,"Number of Dimensions")->valueint;
	}
	
	if(!cJSON_bItemValid(environment,"Subvolume Base Size", cJSON_Number) ||
		cJSON_GetObjectItem(environment,"Subvolume Base Size")->valuedouble <= 0)
	{
		bWarn = true;
		printf("WARNING %d: \"Subvolume Base Size\" not defined or is invalid. Setting to default value of \"1\".\n", numWarn++);
		curSpec->SUBVOL_BASE_SIZE =
			cJSON_GetObjectItem(environment,"Subvolume Base Size")->valuedouble;
	} else
	{
		curSpec->SUBVOL_BASE_SIZE =
			cJSON_GetObjectItem(environment,"Subvolume Base Size")->valuedouble;
	}	
	
	regionSpec = cJSON_GetObjectItem(environment,"Region Specification");
	actorSpec = cJSON_GetObjectItem(environment,"Actor Specification");
	curSpec->NUM_REGIONS =
		cJSON_GetArraySize(regionSpec);
	curSpec->NUM_ACTORS =
		cJSON_GetArraySize(actorSpec);
	if(curSpec->NUM_DIM == 3)
	{
		curSpec->subvol_spec =
			malloc(curSpec->NUM_REGIONS *
			sizeof(struct spec_region3D));
		curSpec->actorSpec =
			malloc(curSpec->NUM_ACTORS *
			sizeof(struct actorStructSpec3D));
		if(curSpec->subvol_spec == NULL
			|| curSpec->actorSpec == NULL)
		{
			puts("Memory could not be allocated to load configuration environment details");
			exit(3);
		}
		for(curArrayItem = 0;
			curArrayItem < curSpec->NUM_REGIONS; curArrayItem++)
		{
			if(!cJSON_bArrayItemValid(regionSpec, curArrayItem, cJSON_Object))
			{
				printf("Error: Region %d is not a valid object.\n", curArrayItem);
				exit(3);
			}
			
			curObj = cJSON_GetArrayItem(regionSpec, curArrayItem);
			
			// Region label
			if(!cJSON_bItemValid(curObj,"Label", cJSON_String))
			{ // Region does not have a defined Label
				bWarn = true;
				printf("WARNING %d: Region %d does not have a defined \"Label\". Assigning empy string.\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].label = '\0';
			} else{
				curSpec->subvol_spec[curArrayItem].label =
					stringAllocate(strlen(cJSON_GetObjectItem(curObj,"Label")->valuestring));
				sprintf(curSpec->subvol_spec[curArrayItem].label, "%s", cJSON_GetObjectItem(curObj,
					"Label")->valuestring);
			}
			
			// Region Parent
			if(!cJSON_bItemValid(curObj,"Parent Label", cJSON_String))
			{ // Region does not have a defined Parent Label
				bWarn = true;
				printf("WARNING %d: Region %d does not have a defined \"Parent Label\". Assigning empy string.\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].parent = '\0';
			} else{
				curSpec->subvol_spec[curArrayItem].parent =
					stringAllocate(strlen(cJSON_GetObjectItem(curObj,"Parent Label")->valuestring));
				sprintf(curSpec->subvol_spec[curArrayItem].parent, "%s", cJSON_GetObjectItem(curObj,
					"Parent Label")->valuestring);
			}
			
			// Region Shape
			if(!cJSON_bItemValid(curObj,"Shape", cJSON_String))
			{ // Region does not have a defined Shape
				bWarn = true;
				printf("WARNING %d: Region %d does not have a defined \"Shape\". Setting to default value \"Rectangular Box\".\n", numWarn++, curArrayItem);
				tempString =
					stringAllocate(strlen("Rectangular Box"));
				strcpy(tempString, "Rectangular Box");
			} else{
				tempString =
					stringAllocate(strlen(cJSON_GetObjectItem(curObj,
					"Shape")->valuestring));
				strcpy(tempString,
					cJSON_GetObjectItem(curObj,"Shape")->valuestring);
			}
			
			if(strcmp(tempString,"Rectangle") == 0)
				curSpec->subvol_spec[curArrayItem].shape = RECTANGLE;
			else if(strcmp(tempString,"Circle") == 0)
				curSpec->subvol_spec[curArrayItem].shape = CIRCLE;
			else if(strcmp(tempString,"Rectangular Box") == 0)
				curSpec->subvol_spec[curArrayItem].shape = RECTANGULAR_BOX;
			else if(strcmp(tempString,"Sphere") == 0)
				curSpec->subvol_spec[curArrayItem].shape = SPHERE;
			else
			{
				bWarn = true;
				printf("WARNING %d: Region %d has an invalid \"Shape\". Setting to default value \"Rectangular Box\".\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].shape = RECTANGULAR_BOX;
			}
			free(tempString);
			
			// Region Position
			if(!cJSON_bItemValid(curObj,"Anchor X Coordinate", cJSON_Number))
			{ // Region does not have a valid Anchor X Coordinate
				bWarn = true;
				printf("WARNING %d: Region %d does not have a valid \"Anchor X Coordinate\". Assigning default value \"0\".\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].xAnch = 0;
			} else
			{
				curSpec->subvol_spec[curArrayItem].xAnch = 
					cJSON_GetObjectItem(curObj, "Anchor X Coordinate")->valuedouble;
			}
			
			if(!cJSON_bItemValid(curObj,"Anchor Y Coordinate", cJSON_Number))
			{ // Region does not have a valid Anchor Y Coordinate
				bWarn = true;
				printf("WARNING %d: Region %d does not have a valid \"Anchor Y Coordinate\". Assigning default value \"0\".\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].yAnch = 0;
			} else
			{
				curSpec->subvol_spec[curArrayItem].yAnch = 
					cJSON_GetObjectItem(curObj, "Anchor Y Coordinate")->valuedouble;
			}
			
			if(!cJSON_bItemValid(curObj,"Anchor Z Coordinate", cJSON_Number))
			{ // Region does not have a valid Anchor Z Coordinate
				bWarn = true;
				printf("WARNING %d: Region %d does not have a valid \"Anchor Z Coordinate\". Assigning default value \"0\".\n", numWarn++, curArrayItem);
				curSpec->subvol_spec[curArrayItem].zAnch = 0;
			} else
			{
				curSpec->subvol_spec[curArrayItem].zAnch = 
					cJSON_GetObjectItem(curObj, "Anchor Z Coordinate")->valuedouble;
			}
			
			if(cJSON_bItemValid(curObj,"Time Step", cJSON_Number))
			{
				bWarn = true;
				printf("WARNING %d: Region %d does not need \"Time Step\" defined. This will be implemented in a future version. Ignoring.\n", numWarn++, curArrayItem);
			}
			
			// Load remaining parameters depending on region shape
			if(curSpec->subvol_spec[curArrayItem].shape == RECTANGULAR_BOX ||
				curSpec->subvol_spec[curArrayItem].shape == RECTANGLE)
			{
				curSpec->subvol_spec[curArrayItem].radius = 0;
				// Check for existence of unnecessary parameters and display
				// warnings if they are defined.
				if(cJSON_bItemValid(curObj,"Radius", cJSON_Number))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Radius\" defined. Ignoring.\n", numWarn++, curArrayItem);
				}
			
				// Width of subvolumes in region (multiple of SUBVOL_BASE_SIZE)
				if(!cJSON_bItemValid(curObj,"Integer Subvolume Size", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Integer Subvolume Size")->valueint < 1)
				{ // Region does not have a valid Integer Subvolume Size
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Integer Subvolume Size\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].sizeRect = 1;
				} else
				{
					curSpec->subvol_spec[curArrayItem].sizeRect = 
						cJSON_GetObjectItem(curObj, "Integer Subvolume Size")->valueint;
				}
				
				// Is region microscopic or mesoscopic?
				if(!cJSON_bItemValid(curObj,"Is Region Microscopic?", cJSON_True))
				{ // Region does not have a valid Is Region Microscopic?
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Is Region Microscopic?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].bMicro = false;
				} else
				{
					curSpec->subvol_spec[curArrayItem].bMicro = 
						cJSON_GetObjectItem(curObj, "Is Region Microscopic?")->valueint;
				}
				
				if(!cJSON_bItemValid(curObj,"Number of Subvolumes Along X", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Number of Subvolumes Along X")->valueint < 1)
				{ // Region does not have a valid Number of Subvolumes Along X
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Number of Subvolumes Along X\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].numX = 1;
				} else
				{
					curSpec->subvol_spec[curArrayItem].numX = 
						cJSON_GetObjectItem(curObj, "Number of Subvolumes Along X")->valueint;
				}
				
				if(!cJSON_bItemValid(curObj,"Number of Subvolumes Along Y", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Number of Subvolumes Along Y")->valueint < 1)
				{ // Region does not have a valid Number of Subvolumes Along Y
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Number of Subvolumes Along Y\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].numY = 1;
				} else
				{
					curSpec->subvol_spec[curArrayItem].numY = 
						cJSON_GetObjectItem(curObj, "Number of Subvolumes Along Y")->valueint;
				}
				
				if(!cJSON_bItemValid(curObj,"Number of Subvolumes Along Z", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Number of Subvolumes Along Z")->valueint < 1)
				{ // Region does not have a valid Number of Subvolumes Along Z
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Number of Subvolumes Along Z\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].numZ = 1;
				} else
				{
					curSpec->subvol_spec[curArrayItem].numZ = 
						cJSON_GetObjectItem(curObj, "Number of Subvolumes Along Z")->valueint;
				}			
			} else // Region is round
			{
				curSpec->subvol_spec[curArrayItem].sizeRect = 0;
				curSpec->subvol_spec[curArrayItem].bMicro = true;
				curSpec->subvol_spec[curArrayItem].numX = 1;
				curSpec->subvol_spec[curArrayItem].numY = 1;
				curSpec->subvol_spec[curArrayItem].numZ = 1;
				// Check for existence of unnecessary parameters and display
				// warnings if they are defined.
				if(cJSON_bItemValid(curObj,"Integer Subvolume Size", cJSON_Number))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Integer Subvolume Size\" defined. Ignoring.\n", numWarn++, curArrayItem);
				}
				if(cJSON_bItemValid(curObj,"Is Region Microscopic?", cJSON_True))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Is Region Microscopic?\" defined. This region must be microscopic. Ignoring.\n", numWarn++, curArrayItem);
				}
				if(cJSON_bItemValid(curObj,"Number of Subvolumes Along X", cJSON_Number))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Number of Subvolumes Along X\" defined. Ignoring.\n", numWarn++, curArrayItem);
				}
				if(cJSON_bItemValid(curObj,"Number of Subvolumes Along Y", cJSON_Number))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Number of Subvolumes Along Y\" defined. Ignoring.\n", numWarn++, curArrayItem);
				}
				if(cJSON_bItemValid(curObj,"Number of Subvolumes Along Z", cJSON_Number))
				{
					bWarn = true;
					printf("WARNING %d: Region %d does not need \"Number of Subvolumes Along Z\" defined. Ignoring.\n", numWarn++, curArrayItem);
				}
			
				// Region radius
				if(!cJSON_bItemValid(curObj,"Radius", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Radius")->valuedouble < 0)
				{ // Region does not have a valid Radius
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Radius\". Assigning value of \"Subvolume Base Size\".\n", numWarn++, curArrayItem);
					curSpec->subvol_spec[curArrayItem].radius = curSpec->SUBVOL_BASE_SIZE;
				} else
				{
					curSpec->subvol_spec[curArrayItem].radius = 
						cJSON_GetObjectItem(curObj, "Radius")->valuedouble;
				}
				
			}
			
			// Override region time step with global one
			curSpec->subvol_spec[curArrayItem].dt = curSpec->DT_MICRO;
			//curSpec->subvol_spec[curArrayItem].dt = 
			//	cJSON_GetObjectItem(curObj,
			//	"Time Step")->valuedouble;
		}
		for(curArrayItem = 0;
			curArrayItem < curSpec->NUM_ACTORS; curArrayItem++)
		{			
			if(!cJSON_bArrayItemValid(actorSpec, curArrayItem, cJSON_Object))
			{
				printf("Error: Actor %d is not a valid object.\n", curArrayItem);
				exit(3);
			}
			
			curObj = cJSON_GetArrayItem(actorSpec, curArrayItem);
			
			if(!cJSON_bItemValid(curObj,"Shape", cJSON_String))
			{ // Actor does not have a defined Shape
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a defined \"Shape\". Setting to default value \"Rectangular Box\".\n", numWarn++, curArrayItem);
				tempString =
					stringAllocate(strlen("Rectangular Box"));
				strcpy(tempString, "Rectangular Box");
			} else{
				tempString =
					stringAllocate(strlen(cJSON_GetObjectItem(curObj,
					"Shape")->valuestring));
				strcpy(tempString,
					cJSON_GetObjectItem(curObj,"Shape")->valuestring);
			}
			
			if(strcmp(tempString,"Rectangle") == 0)
			{
				curSpec->actorSpec[curArrayItem].shape = RECTANGLE;
				arrayLen = 6;
			}
			else if(strcmp(tempString,"Circle") == 0)
			{
				curSpec->actorSpec[curArrayItem].shape = CIRCLE;
				arrayLen = 4;
			}
			else if(strcmp(tempString,"Rectangular Box") == 0)
			{
				curSpec->actorSpec[curArrayItem].shape = RECTANGULAR_BOX;
				arrayLen = 6;
			}
			else if(strcmp(tempString,"Sphere") == 0)
			{
				curSpec->actorSpec[curArrayItem].shape = SPHERE;
				arrayLen = 4;
			}
			else
			{
				bWarn = true;
				printf("WARNING %d: Actor %d has an invalid \"Shape\". Setting to default value \"Rectangular Box\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].shape = RECTANGULAR_BOX;
				arrayLen = 6;
			}
			
			if(!cJSON_bItemValid(curObj,"Outer Boundary", cJSON_Array) ||
				cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Outer Boundary")) != arrayLen)
			{
				bWarn = true;
				printf("WARNING %d: Actor %d has a missing or invalid \"Outer Boundary\". Setting to default value all \"0\"s.\n", numWarn++, curArrayItem);
				for(i = 0; i < arrayLen; i++)
				{
					curSpec->actorSpec[curArrayItem].boundary[i] = 0;
				}
			} else
			{
				curObjInner = cJSON_GetObjectItem(curObj,"Outer Boundary");
				for(i = 0; i < arrayLen; i++)
				{
					if(!cJSON_bArrayItemValid(curObjInner,i, cJSON_Number))
					{
						bWarn = true;
						printf("WARNING %d: Actor %d has an invalid \"Outer Boundary\" parameter %d. Setting to default value \"0\".\n", numWarn++, curArrayItem, i);
						curSpec->actorSpec[curArrayItem].boundary[i] = 0;
					} else
					{
						curSpec->actorSpec[curArrayItem].boundary[i] =
							cJSON_GetArrayItem(curObjInner, i)->valuedouble;
					}					
				}
			}
			
			// Add r^2 term for spherical boundaries
			if(strcmp(tempString,"Sphere") == 0)
			{
				curSpec->actorSpec[curArrayItem].boundary[4] =
					squareDBL(curSpec->actorSpec[curArrayItem].boundary[3]);
			}
			free(tempString);
			
			if(!cJSON_bItemValid(curObj,"Is Actor Active?", cJSON_True))
			{ // Actor does not have a valid Is Actor Active?
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a valid \"Is Actor Active?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].bActive = false;
			} else
			{
				curSpec->actorSpec[curArrayItem].bActive = 
					cJSON_GetObjectItem(curObj, "Is Actor Active?")->valueint;
			}
			
			if(!cJSON_bItemValid(curObj,"Start Time", cJSON_Number))
			{ // Actor does not have a valid Start Time
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a valid \"Start Time\". Assigning default value \"0\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].startTime = 0;
			} else
			{
				curSpec->actorSpec[curArrayItem].startTime = 
					cJSON_GetObjectItem(curObj, "Start Time")->valuedouble;
			}
			
			if(!cJSON_bItemValid(curObj,"Is There Max Number of Actions?", cJSON_True))
			{ // Actor does not have a valid Is There Max Number of Actions?
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a valid \"Is There Max Number of Actions?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].bMaxAction = false;
			} else
			{
				curSpec->actorSpec[curArrayItem].bMaxAction = 
					cJSON_GetObjectItem(curObj, "Is There Max Number of Actions?")->valueint;
			}
			
			if(curSpec->actorSpec[curArrayItem].bMaxAction)
			{
				if(!cJSON_bItemValid(curObj,"Max Number of Actions", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Max Number of Actions")->valueint < 1)
				{ // Region does not have a valid Max Number of Actions
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Max Number of Actions\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].numMaxAction = 1;
				} else
				{
					curSpec->actorSpec[curArrayItem].numMaxAction = 
						cJSON_GetObjectItem(curObj, "Max Number of Actions")->valueint;
				}
			} else
			{
				curSpec->actorSpec[curArrayItem].numMaxAction = 0;
			}
			
			if(!cJSON_bItemValid(curObj,"Is Actor Independent?", cJSON_True))
			{ // Actor does not have a valid Is Actor Independent?
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a valid \"Is Actor Independent?\". Assigning default value \"true\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].bIndependent = true;
			} else
			{
				curSpec->actorSpec[curArrayItem].bIndependent = 
					cJSON_GetObjectItem(curObj, "Is Actor Independent?")->valueint;
			}
			
			if(!cJSON_bItemValid(curObj,"Action Interval", cJSON_Number) ||
				cJSON_GetObjectItem(curObj,"Action Interval")->valuedouble <= 0)
			{ // Actor does not have a valid Action Interval
				bWarn = true;
				printf("WARNING %d: Actor %d does not have a valid \"Action Interval\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
				curSpec->actorSpec[curArrayItem].actionInterval = 1.;
			} else
			{
				curSpec->actorSpec[curArrayItem].actionInterval = 
					cJSON_GetObjectItem(curObj, "Action Interval")->valuedouble;
			}
			
			if(curSpec->actorSpec[curArrayItem].bActive)
			{ // Actor is active. Check for all active parameters
				if(!cJSON_bItemValid(curObj,"Random Number of Molecules?", cJSON_True))
				{ // Actor does not have a valid value for Random Number of Molecules?
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid value for \"Random Number of Molecules?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bNumReleaseRand = false;
				} else
				{
					curSpec->actorSpec[curArrayItem].bNumReleaseRand = 
						cJSON_GetObjectItem(curObj, "Random Number of Molecules?")->valueint;
				}
				
				if(!cJSON_bItemValid(curObj,"Random Molecule Release Times?", cJSON_True))
				{ // Actor does not have a valid value for Random Molecule Release Times?
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid value for \"Random Molecule Release Times?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bTimeReleaseRand = false;
				} else
				{
					curSpec->actorSpec[curArrayItem].bTimeReleaseRand = 
						cJSON_GetObjectItem(curObj, "Random Molecule Release Times?")->valueint;
				}
			
				if(!cJSON_bItemValid(curObj,"Release Interval", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Release Interval")->valuedouble < 0)
				{ // Actor does not have a valid Action Interval
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Release Interval\". Assigning default value \"0\" seconds.\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].releaseInterval = 0.;
				} else
				{
					curSpec->actorSpec[curArrayItem].releaseInterval = 
						cJSON_GetObjectItem(curObj, "Release Interval")->valuedouble;
				}
			
				if(!cJSON_bItemValid(curObj,"Slot Interval", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Slot Interval")->valuedouble < 0)
				{ // Actor does not have a valid Action Interval
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Slot Interval\". Assigning default value \"0\" seconds.\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].slotInterval = 0.;
				} else
				{
					curSpec->actorSpec[curArrayItem].slotInterval = 
						cJSON_GetObjectItem(curObj, "Slot Interval")->valuedouble;
				}
				
				/*
				if(!cJSON_bItemValid(curObj,"Bits Random?", cJSON_True))
				{ // Actor does not have a valid value for Bits Random?
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid value for \"Bits Random?\". Assigning default value \"true\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bRandBits = true;
				} else
				{
					curSpec->actorSpec[curArrayItem].bRandBits = 
						cJSON_GetObjectItem(curObj, "Bits Random?")->valueint;
				}*/
				curSpec->actorSpec[curArrayItem].bRandBits = true; // NOTE: CURRENTLY MUST BE TRUE
			
				if(!cJSON_bItemValid(curObj,"Probability of Bit 1", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Probability of Bit 1")->valuedouble < 0. ||
					cJSON_GetObjectItem(curObj,"Probability of Bit 1")->valuedouble > 1.)
				{ // Actor does not have a valid Action Interval
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Probability of Bit 1\". Assigning default value \"0.5\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].probOne = 0.5;
				} else
				{
					curSpec->actorSpec[curArrayItem].probOne = 
						cJSON_GetObjectItem(curObj, "Probability of Bit 1")->valuedouble;
				}
					
			
				if(!cJSON_bItemValid(curObj,"Modulation Scheme", cJSON_String))
				{ // Actor does not have a defined Modulation Scheme
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a defined \"Modulation Scheme\". Setting to default value \"CSK\".\n", numWarn++, curArrayItem);
					tempString =
						stringAllocate(strlen("CSK"));
					strcpy(tempString, "CSK");
				} else{
					tempString =
						stringAllocate(strlen(cJSON_GetObjectItem(curObj,
						"Modulation Scheme")->valuestring));
					strcpy(tempString,
						cJSON_GetObjectItem(curObj,"Modulation Scheme")->valuestring);
				}			
				
				if(strcmp(tempString,"CSK") == 0)
					curSpec->actorSpec[curArrayItem].modScheme = CSK;
				else
				{
					bWarn = true;
					printf("WARNING %d: Actor %d has an invalid \"Modulation Scheme\". Setting to default value \"CSK\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].modScheme = CSK;
				}
				free(tempString);
			
				if(!cJSON_bItemValid(curObj,"Modulation Bits", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Modulation Bits")->valueint < 1)
				{ // Region does not have a valid Modulation Bits
					bWarn = true;
					printf("WARNING %d: Region %d does not have a valid \"Modulation Bits\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].modBits = 1;
				} else
				{
					curSpec->actorSpec[curArrayItem].modBits = 
						cJSON_GetObjectItem(curObj, "Modulation Bits")->valueint;
				}
			
				if(!cJSON_bItemValid(curObj,"Modulation Strength", cJSON_Number) ||
					cJSON_GetObjectItem(curObj,"Modulation Strength")->valuedouble <= 0.)
				{ // Actor does not have a valid Modulation Strength
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Modulation Strength\". Assigning default value \"1\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].modStrength = 1.;
				} else
				{
					curSpec->actorSpec[curArrayItem].modStrength = 
						cJSON_GetObjectItem(curObj, "Modulation Strength")->valuedouble;
				}
				
				if(!cJSON_bItemValid(curObj,"Is Molecule Type Released?", cJSON_Array) ||
					cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Is Molecule Type Released?")) != curSpec->NUM_MOL_TYPES)
				{ // Config file does not list a valid Is Molecule Type Released? array
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Is Molecule Type Released?\" array or not of correct length. Assigning default value \"true\" to first molecule type.\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bReleaseMol[0] = true;
					for(curMolType = 1; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						curSpec->actorSpec[curArrayItem].bReleaseMol[curMolType] = false;
					}
				} else{
					curObjInner = cJSON_GetObjectItem(curObj, "Is Molecule Type Released?");
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						if(!cJSON_bArrayItemValid(curObjInner,curMolType, cJSON_True))
						{
							bWarn = true;
							printf("WARNING %d: \"Is Molecule Type Released?\" %d of Actor %d not defined or has an invalid value. Assigning default value of \"false\".\n", numWarn++, curMolType, curArrayItem);
							curSpec->actorSpec[curArrayItem].bReleaseMol[curMolType] = false;
						} else
						{
							curSpec->actorSpec[curArrayItem].bReleaseMol[curMolType] =
								cJSON_GetArrayItem(curObjInner, curMolType)->valueint;
						}
					}
				}
			} else
			{ // Actor is passive. Check for all passive parameters
			
				if(!cJSON_bItemValid(curObj,"Is Actor Activity Recorded?", cJSON_True))
				{ // Actor does not have a valid Is Actor Activity Recorded?
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Is Actor Activity Recorded?\". Assigning default value \"true\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bWrite = true;
				} else
				{
					curSpec->actorSpec[curArrayItem].bWrite = 
						cJSON_GetObjectItem(curObj, "Is Actor Activity Recorded?")->valueint;
				}
				
				if(!cJSON_bItemValid(curObj,"Is Time Recorded with Activity?", cJSON_True))
				{ // Actor does not have a valid Is Time Recorded with Activity?
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Is Time Recorded with Activity?\". Assigning default value \"false\".\n", numWarn++, curArrayItem);
					curSpec->actorSpec[curArrayItem].bRecordTime = false;
				} else
				{
					curSpec->actorSpec[curArrayItem].bRecordTime = 
						cJSON_GetObjectItem(curObj, "Is Time Recorded with Activity?")->valueint;
				}
				
				
				if(!cJSON_bItemValid(curObj,"Is Molecule Type Observed?", cJSON_Array) ||
					cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Is Molecule Type Observed?")) != curSpec->NUM_MOL_TYPES)
				{ // Config file does not list a valid Is Molecule Type Observed? array
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Is Molecule Type Observed?\" array or not of correct length. Assigning default value \"true\" to each molecule type.\n", numWarn++, curArrayItem);
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						curSpec->actorSpec[curArrayItem].bRecordMol[curMolType] = true;
					}
				} else{
					curObjInner = cJSON_GetObjectItem(curObj, "Is Molecule Type Observed?");
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						if(!cJSON_bArrayItemValid(curObjInner,curMolType, cJSON_True))
						{
							bWarn = true;
							printf("WARNING %d: \"Is Molecule Type Observed?\" %d of Actor %d not defined or has an invalid value. Assigning default value of \"true\".\n", numWarn++, curMolType, curArrayItem);
							curSpec->actorSpec[curArrayItem].bRecordMol[curMolType] = true;
						} else
						{
							curSpec->actorSpec[curArrayItem].bRecordMol[curMolType] =
								cJSON_GetArrayItem(curObjInner, curMolType)->valueint;
						}
					}
				}
				
				if(!cJSON_bItemValid(curObj,"Is Molecule Position Observed?", cJSON_Array) ||
					cJSON_GetArraySize(cJSON_GetObjectItem(curObj,"Is Molecule Position Observed?")) != curSpec->NUM_MOL_TYPES)
				{ // Config file does not list a valid Is Molecule Position Observed? array
					bWarn = true;
					printf("WARNING %d: Actor %d does not have a valid \"Is Molecule Position Observed?\" array or not of correct length. Assigning default value \"false\" to each molecule type.\n", numWarn++, curArrayItem);
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						curSpec->actorSpec[curArrayItem].bRecordPos[curMolType] = false;
					}
				} else{
					curObjInner = cJSON_GetObjectItem(curObj, "Is Molecule Position Observed?");
					for(curMolType = 0; curMolType < curSpec->NUM_MOL_TYPES; curMolType++)
					{
						if(!cJSON_bArrayItemValid(curObjInner,curMolType, cJSON_True))
						{
							bWarn = true;
							printf("WARNING %d: \"Is Molecule Position Observed?\" %d of Actor %d not defined or has an invalid value. Assigning default value of \"false\".\n", numWarn++, curMolType, curArrayItem);
							curSpec->actorSpec[curArrayItem].bRecordPos[curMolType] = false;
						} else
						{
							curSpec->actorSpec[curArrayItem].bRecordPos[curMolType] =
								cJSON_GetArrayItem(curObjInner, curMolType)->valueint;
						}
					}
				}
			}
		}
	}
	
	// Cleanup
	cJSON_Delete(configJSON);
	free(configContent);
	free(configNameFull);
	
	// Pause for warnings if needed
	printf("Configuration file has %d warning(s). ", numWarn);
	if(bWarn  && !bWarnOverride)
	{
		printf("Press \'Enter\' to continue the simulation or \'q\'+\'Enter\' to quit.\n");
		ch = getchar();
		if(ch == 'q')
			exit(3);
	} else printf("\n");
}

// Release memory allocated to configuration settings
void deleteConfig3D(struct simSpec3D curSpec)
{
	unsigned short curRegion;
	unsigned short curRxn;
	
	free(curSpec.DIFF_COEF);
	free(curSpec.actorSpec);
	free(curSpec.OUTPUT_NAME);
	
	for(curRxn = 0; curRxn < curSpec.MAX_RXNS; curRxn++)
	{
		for(curRegion = 0; curRegion < curSpec.chem_rxn[curRxn].numRegionExceptions; curRegion++)
		{
			free(curSpec.chem_rxn[curRxn].regionExceptionLabel[curRegion]);
		}
		free(curSpec.chem_rxn[curRxn].regionExceptionLabel);
	}
	free(curSpec.chem_rxn);
	
	for(curRegion = 0; curRegion < curSpec.NUM_REGIONS; curRegion++)
	{
		free(curSpec.subvol_spec[curRegion].label);
		free(curSpec.subvol_spec[curRegion].parent);
	}
	free(curSpec.subvol_spec);
}

// Initialize the simulation output file
void initializeOutput3D(FILE ** out,
	FILE ** outSummary,
	const char * CONFIG_NAME,
	const struct simSpec3D curSpec)
{
	time_t timer;
	char timeBuffer[26];
	struct tm* timeInfo;
	cJSON * root;
	char * outText;
	char * outDir = "../results/";
	char * outPre = "../results/summary_";
	char * outputNameFull;
	char * outputSummaryNameFull;
	unsigned int dirLength, nameLength;
	
	time(&timer);
	timeInfo = localtime(&timer);
	
	strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", timeInfo);
	
	// Construct full name of output file
	dirLength = strlen(outDir);
	nameLength = strlen(curSpec.OUTPUT_NAME);
	outputNameFull = malloc(dirLength + nameLength + 1);
	outputSummaryNameFull = malloc(dirLength + nameLength + 19);
	if(outputNameFull == NULL
		|| outputSummaryNameFull == NULL)
	{
		puts("Memory could not be allocated to build the configuration file name");
		exit(3);
	}
	outputNameFull[0] = '\0'; // Initialize full name to the empty string
	outputSummaryNameFull[0] = '\0';
	strcat(outputNameFull,outDir);
	strcat(outputNameFull,curSpec.OUTPUT_NAME);
	strcat(outputSummaryNameFull,outPre);
	strcat(outputSummaryNameFull,curSpec.OUTPUT_NAME);
	
	if((*out = fopen(outputNameFull, "w")) == NULL)
	{
		fprintf(stderr,"Can't create output file %s.\n",curSpec.OUTPUT_NAME);
		exit(3);
	}
	if((*outSummary = fopen(outputSummaryNameFull, "w")) == NULL)
	{
		fprintf(stderr,"Can't create output summary file %s.\n",curSpec.OUTPUT_NAME);
		exit(3);
	}
	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "ConfigFile", CONFIG_NAME);
	cJSON_AddNumberToObject(root, "SEED", curSpec.SEED);
	cJSON_AddNumberToObject(root, "NumRepeat", curSpec.NUM_REPEAT);
	cJSON_AddStringToObject(root, "StartTime", timeBuffer);
	
	outText = cJSON_Print(root);
	fprintf(*outSummary, "%s", outText);
	fprintf(*outSummary, "\n");
	cJSON_Delete(root);
	free(outText);
	free(outputNameFull);
	free(outputSummaryNameFull);	
}

char * stringAllocate(long stringLength)
{
	char * string = malloc(stringLength+1);
	if(string == NULL)
	{
		puts("Error in memory allocation for string copy");
		exit(3);
	}
	
	return string;
}

// Print simulation output from one realization
void printOneTextRealization3D(FILE * out,
	const struct simSpec3D curSpec,
	unsigned int curRepeat,
	ListObs3D observationArray[],
	short numActorRecord,
	short * actorRecordID,
	short NUM_ACTORS_ACTIVE,
	const struct actorStruct3D actorCommonArray[],
	const struct actorActiveStruct3D actorActiveArray[],
	const struct actorPassiveStruct3D actorPassiveArray[],	
	uint32_t maxActiveBits[],
	uint32_t maxPassiveObs[])
{
	short curActor, curActorPassive, curActorRecord, curActorActive;
	char * outText;
	NodeData * curData;
	NodeObs3D * curObs;
	unsigned short curMolInd, curMolType;
	ListMol3D * curMolList;
	NodeMol3D * curMolNode;
	uint32_t curActiveBits, curPassiveObs;
	
	fprintf(out, "Realization %u:\n", curRepeat);
	
	// Record active actor binary data
	for(curActorActive = 0; curActorActive < NUM_ACTORS_ACTIVE;
		curActorActive++)
	{
		curData =  actorActiveArray[curActorActive].binaryData.head;
		curActor = actorActiveArray[curActorActive].actorID;
		curActiveBits = 0;
		fprintf(out, "\tActiveActor %u:\n\t\t", curActor);
		while(curData != NULL)
		{
			fprintf(out, "%u ", curData->item.bit);
			curData = curData->next;
			curActiveBits++;
		}		
		fprintf(out, "\n");
		
		if (curActiveBits > maxActiveBits[curActorActive])
			maxActiveBits[curActorActive] = curActiveBits;
	}
	
	// Record observations by passive actors that are being recorded
	for(curActorRecord = 0; curActorRecord < numActorRecord; curActorRecord++)
	{
		// Actor in common actor list is actorRecordID[curActorRecord]
		curActor = actorRecordID[curActorRecord];
		curObs = (&observationArray[curActorRecord])->head;
		fprintf(out, "\tPassiveActor %u:\n", curActor);
		
		// Preliminary scan to find number of observations and compare
		// with largest number of observations made thus far in any realization
		curPassiveObs = 0;
		while(curObs != NULL)
		{
			curPassiveObs++;
			curObs = curObs->next;
		}		
		if (curPassiveObs > maxPassiveObs[curActorRecord])
			maxPassiveObs[curActorRecord] = curPassiveObs;
		
		// Record actor observation times (if being recorded)
		curObs = (&observationArray[curActorRecord])->head;
		if (actorCommonArray[curActor].spec.bRecordTime)
		{
			fprintf(out, "\t\tTime:\n\t\t\t");
			while(curObs != NULL)
			{
				fprintf(out, "%.4e ", curObs->item.paramDouble[0]);
				curObs = curObs->next;
			}
			fprintf(out, "\n");
		}
		
		// Record observations associated with each type of molecule being recorded
		curActorPassive = actorCommonArray[curActor].passiveID;
		for(curMolInd = 0;
			curMolInd < actorPassiveArray[curActorPassive].numMolRecordID;
			curMolInd++)
		{
			curMolType = actorPassiveArray[curActorPassive].molRecordID[curMolInd];
			fprintf(out, "\t\tMolID %u:\n\t\t\tCount:\n\t\t\t\t", curMolType);
			
			// Record molecule counts made by observer
			curObs = (&observationArray[curActorRecord])->head;
			while(curObs != NULL)
			{
				fprintf(out, "%" PRIu64 " ", curObs->item.paramUllong[curMolInd]);
				curObs = curObs->next;
			}
			fprintf(out, "\n");
			
			// Record molecule coordinates if specified
			if (actorCommonArray[curActor].spec.bRecordPos[curMolType])
			{
				curObs = (&observationArray[curActorRecord])->head;
				fprintf(out, "\t\t\tPosition:");
				while(curObs != NULL)
				{		
					fprintf(out, "\n\t\t\t\t");
					// Each observation will have the positions of some number of molecules
					fprintf(out, "(");
					curMolList = curObs->item.molPos[curMolInd];
					if(!isListMol3DEmpty(curMolList))
					{
						curMolNode = *curMolList;
						while(curMolNode != NULL)
						{
							fprintf(out, "(%e, %e, %e) ",
								curMolNode->item.x, curMolNode->item.y, curMolNode->item.z);
							curMolNode = curMolNode->next;
						}
					}						
					fprintf(out, ")");		
					curObs = curObs->next;
				}
				fprintf(out, "\n");
			}
		}
	}
	fprintf(out, "\n");
}

// Print end of simulation data
void printTextEnd3D(FILE * out,	
	short NUM_ACTORS_ACTIVE,
	short numActorRecord,
	const struct actorStruct3D actorCommonArray[],
	const struct actorActiveStruct3D actorActiveArray[],
	const struct actorPassiveStruct3D actorPassiveArray[],
	short * actorRecordID,
	uint32_t maxActiveBits[],
	uint32_t maxPassiveObs[])
{
	time_t timer;
	char timeBuffer[26];
	struct tm* timeInfo;
	short curActor, curPassive, curActorRecord;
	cJSON * root;
	cJSON * curArray, *curItem, *newItem, *newActor, *innerArray;
	char * outText;
	unsigned short curMolInd;
	
	time(&timer);
	timeInfo = localtime(&timer);
	
	strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", timeInfo);
	
	root = cJSON_CreateObject();
	
	// Store information about the active actors
	cJSON_AddNumberToObject(root, "NumberActiveActor", NUM_ACTORS_ACTIVE);
	cJSON_AddItemToObject(root, "ActiveInfo", curArray=cJSON_CreateArray());
	for(curActor = 0; curActor < NUM_ACTORS_ACTIVE; curActor++)
	{
		newActor = cJSON_CreateObject();
		cJSON_AddNumberToObject(newActor, "ID",
			actorActiveArray[curActor].actorID);
		cJSON_AddNumberToObject(newActor, "MaxBitLength",
			maxActiveBits[curActor]);
		cJSON_AddItemToArray(curArray, newActor);
	}
	
	// Store information about the passive actors that were recorded
	cJSON_AddNumberToObject(root, "NumberPassiveRecord", numActorRecord);	
	cJSON_AddItemToObject(root, "RecordInfo", curArray=cJSON_CreateArray());
	for(curActorRecord = 0; curActorRecord < numActorRecord; curActorRecord++)
	{
		curActor = actorRecordID[curActorRecord];
		curPassive = actorCommonArray[curActor].passiveID;
		newActor = cJSON_CreateObject();
		// Record Passive Actor IDs that are being recorded
		cJSON_AddNumberToObject(newActor, "ID",
			curActor);
		cJSON_AddNumberToObject(newActor, "bRecordTime",
			actorCommonArray[curActor].spec.bRecordTime);
		// Record maximum number of observations made by each recorded actor
		cJSON_AddNumberToObject(newActor, "MaxCountLength",
			maxPassiveObs[curActorRecord]);
		cJSON_AddNumberToObject(newActor, "NumMolTypeObs",
			actorPassiveArray[curPassive].numMolRecordID);
		cJSON_AddItemToObject(newActor, "MolObsID", innerArray=cJSON_CreateArray());
		for(curMolInd = 0; curMolInd < actorPassiveArray[curPassive].numMolRecordID;
			curMolInd++)
		{
			cJSON_AddItemToArray(innerArray,
				cJSON_CreateNumber(
				actorPassiveArray[curPassive].molRecordID[curMolInd]));
		}
		cJSON_AddItemToObject(newActor, "bRecordPos", innerArray=cJSON_CreateArray());
		for(curMolInd = 0; curMolInd < actorPassiveArray[curPassive].numMolRecordID;
			curMolInd++)
		{
			cJSON_AddItemToArray(innerArray,
				cJSON_CreateNumber(
				actorCommonArray[curActor].spec.bRecordPos[actorPassiveArray[curPassive].molRecordID[curMolInd]]));
		}		
		cJSON_AddItemToArray(curArray, newActor);
	}
	
	cJSON_AddStringToObject(root, "EndTime", timeBuffer);
	
	outText = cJSON_Print(root);
	fprintf(out, "%s", outText);
	cJSON_Delete(root);
	free(outText);
}
