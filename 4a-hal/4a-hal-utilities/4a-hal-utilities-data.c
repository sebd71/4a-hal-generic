/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author Jonathan Aillet <jonathan.aillet@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <wrap-json.h>

#include "4a-hal-utilities-data.h"

/*******************************************************************************
 *		Specfic Hal controller streams data handling functions	       *
 ******************************************************************************/

uint8_t HalUtlRemoveAllCtlHalStreamsData(struct CtlHalStreamsDataT *ctlHalStreamsData)
{
	unsigned int cpt;

	if(! ctlHalStreamsData)
		return -1;

	if(! ctlHalStreamsData->count)
		return -2;

	for(cpt = 0; cpt < ctlHalStreamsData->count; cpt++) {
		free(ctlHalStreamsData->data[cpt].name);
		free(ctlHalStreamsData->data[cpt].streamCardId);
	}

	free(ctlHalStreamsData->data);

	return 0;
}

/*******************************************************************************
 *		Specfic Hal data handling functions			       *
 ******************************************************************************/

struct SpecificHalData *HalUtlAddHalApiToHalList(struct HalMgrData *HalMgrGlobalData)
{
	struct SpecificHalData *currentApi;

	if(! HalMgrGlobalData)
		return NULL;

	currentApi = HalMgrGlobalData->first;

	if(! currentApi) {
		currentApi = (struct SpecificHalData *) calloc(1, sizeof(struct SpecificHalData));
		if(! currentApi)
			return NULL;

		HalMgrGlobalData->first = currentApi;
	}
	else {
		while(currentApi->next)
			currentApi = currentApi->next;

		currentApi->next = calloc(1, sizeof(struct SpecificHalData));
		if(! currentApi)
			return NULL;

		currentApi = currentApi->next;
	}

	memset(currentApi, 0, sizeof(struct SpecificHalData));

	return currentApi;
}

uint8_t HalUtlRemoveSelectedHalFromList(struct HalMgrData *HalMgrGlobalData, struct SpecificHalData *apiToRemove)
{
	struct SpecificHalData *currentApi, *matchingApi;

	if(! HalMgrGlobalData || ! apiToRemove)
		return -1;

	currentApi = HalMgrGlobalData->first;

	if(currentApi == apiToRemove) {
		HalMgrGlobalData->first = currentApi->next;
		matchingApi = currentApi;
	}
	else {
		while(currentApi && currentApi->next != apiToRemove)
			currentApi = currentApi->next;

		if(currentApi) {
			matchingApi = currentApi->next;
			currentApi->next = currentApi->next->next;
		}
		else {
			return -2;
		}
	}

	if(! matchingApi->internal) {
		free(matchingApi->apiName);
		free(matchingApi->sndCard);
		free(matchingApi->info);
		free(matchingApi->author);
		free(matchingApi->version);
		free(matchingApi->date);
	}
	else {
		HalUtlRemoveAllCtlHalStreamsData(&matchingApi->ctlHalSpecificData->ctlHalStreamsData);

		free(matchingApi->ctlHalSpecificData);
	}

	free(matchingApi);

	return 0;
}

uint64_t HalUtlRemoveAllHalFromList(struct HalMgrData *HalMgrGlobalData)
{
	uint8_t ret;
	uint64_t CtlHalApiRemoved = 0;

	while(HalMgrGlobalData->first) {
		ret = HalUtlRemoveSelectedHalFromList(HalMgrGlobalData, HalMgrGlobalData->first);
		if(ret)
			return (uint64_t) ret;

		CtlHalApiRemoved++;
	}

	return CtlHalApiRemoved;
}

uint64_t HalUtlGetNumberOfHalInList(struct HalMgrData *HalMgrGlobalData)
{
	uint64_t numberOfCtlHal = 0;
	struct SpecificHalData *currentApi;

	if(! HalMgrGlobalData)
		return -1;

	currentApi = HalMgrGlobalData->first;

	while(currentApi) {
		currentApi = currentApi->next;
		numberOfCtlHal++;
	}

	return numberOfCtlHal;
}

struct SpecificHalData *HalUtlSearchHalDataByApiName(struct HalMgrData *HalMgrGlobalData, char *apiName)
{
	struct SpecificHalData *currentApi;

	if(! HalMgrGlobalData || ! apiName)
		return NULL;

	currentApi = HalMgrGlobalData->first;

	while(currentApi) {
		if(! strcmp(apiName, currentApi->apiName))
			return currentApi;

		currentApi = currentApi->next;
	}

	return NULL;
}

/*******************************************************************************
 *		Hal Manager data handling functions			       *
 ******************************************************************************/

uint8_t HalUtlInitializeHalMgrData(afb_dynapi *apiHandle, struct HalMgrData *HalMgrGlobalData, char *apiName, char *info)
{
	if(! apiHandle || ! HalMgrGlobalData || ! apiName || ! info)
		return -1;

	// Allocate and fill apiName and info strings
	HalMgrGlobalData->apiName = strdup(apiName);
	if(! HalMgrGlobalData->apiName)
		return -2;

	HalMgrGlobalData->info = strdup(info);
	if(! HalMgrGlobalData->apiName)
		return -3;

	HalMgrGlobalData->apiHandle = apiHandle;

	return 0;
}

void HalUtlRemoveHalMgrData(struct HalMgrData *HalMgrGlobalData)
{
	if(! HalMgrGlobalData)
		return;

	if(HalMgrGlobalData->first)
		HalUtlRemoveAllHalFromList(HalMgrGlobalData);

	free(HalMgrGlobalData->apiName);
	free(HalMgrGlobalData->info);

	free(HalMgrGlobalData);
}