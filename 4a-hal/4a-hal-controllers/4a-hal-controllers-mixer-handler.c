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

#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-mixer-handler.h"
#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers handle mixer response function		       *
 ******************************************************************************/

int HalCtlsHandleMixerAttachResponse(AFB_ReqT request, struct CtlHalStreamsDataT *currentHalStreamsData, json_object *mixerResponseJ)
{
	int err = (int) MIXER_NO_ERROR;
	unsigned int idx;

	char *currentStreamName, *currentStreamCardId;

	afb_dynapi *apiHandle;

	struct HalUtlApiVerb *CtlHalDynApiStreamVerbs;

	json_object *currentStreamJ;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_REQUEST_WARNING(request, "%s: Can't get current hal api handle", __func__);
		return (int) MIXER_ERROR_API_UNAVAILABLE;
	}

	switch(json_object_get_type(mixerResponseJ)) {
		case json_type_object:
			currentHalStreamsData->count = 1;
			break;
		case json_type_array:
			currentHalStreamsData->count = json_object_array_length(mixerResponseJ);
			break;
		default:
			currentHalStreamsData->count = 0;
			AFB_REQUEST_WARNING(request, "%s: no streams returned", __func__);
			return (int) MIXER_ERROR_NO_STREAMS;
	}

	currentHalStreamsData->data = (struct CtlHalStreamData *) calloc(currentHalStreamsData->count, sizeof(struct CtlHalStreamData));

	CtlHalDynApiStreamVerbs = alloca((currentHalStreamsData->count + 1) * sizeof(struct HalUtlApiVerb));
	memset(CtlHalDynApiStreamVerbs, '\0', (currentHalStreamsData->count + 1) * sizeof(struct HalUtlApiVerb));
	CtlHalDynApiStreamVerbs[currentHalStreamsData->count].verb = NULL;

	for(idx = 0; idx < currentHalStreamsData->count; idx++) {
		if(currentHalStreamsData->count > 1)
			currentStreamJ = json_object_array_get_idx(mixerResponseJ, (int) idx);
		else
			currentStreamJ = mixerResponseJ;

		if(wrap_json_unpack(currentStreamJ, "{s:s}", "uid", &currentStreamName)) {
			AFB_REQUEST_WARNING(request, "%s: can't find name in current stream object", __func__);
			err += (int) MIXER_ERROR_STREAM_NAME_UNAVAILABLE;
		}
		else if(wrap_json_unpack(currentStreamJ, "{s:s}", "alsa", &currentStreamCardId)) {
			AFB_REQUEST_WARNING(request, "%s: can't find card id in current stream object", __func__);
			err += (int) MIXER_ERROR_STREAM_CARDID_UNAVAILABLE;
		}
		else {
			currentHalStreamsData->data[idx].name = strdup(currentStreamName);
			currentHalStreamsData->data[idx].streamCardId = strdup(currentStreamCardId);

			CtlHalDynApiStreamVerbs[idx].verb = currentStreamName;
			CtlHalDynApiStreamVerbs[idx].callback = HalCtlsActionOnStream;
			CtlHalDynApiStreamVerbs[idx].info = "Peform action on this stream";
		}
	}

	if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStreamVerbs)) {
		AFB_REQUEST_WARNING(request, "%s: error while creating verbs for streams", __func__);
		err += (int) MIXER_ERROR_COULDNT_ADD_STREAMS_AS_VERB;
	}

	return err;
}