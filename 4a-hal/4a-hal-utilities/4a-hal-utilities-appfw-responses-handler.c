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

#include "4a-hal-utilities-appfw-responses-handler.h"

/*******************************************************************************
 *		Handle application framework response function		       *
 ******************************************************************************/

enum CallError HalUtlHandleAppFwCallError(afb_dynapi *apiHandle, char *apiCalled, char *verbCalled, json_object *callReturnJ, char **returnedStatus, char **returnedInfo)
{
	json_object *returnedRequestJ, *returnedStatusJ, *returnedInfoJ;

	if(! apiHandle || ! apiCalled || ! verbCalled || ! callReturnJ)
		return CALL_ERROR_INVALID_ARGS;

	if(! json_object_object_get_ex(callReturnJ, "request", &returnedRequestJ)) {
		AFB_DYNAPI_WARNING(apiHandle, "Couldn't get response request object");
		return CALL_ERROR_REQUEST_UNAVAILABLE;
	}

	if(! json_object_is_type(returnedRequestJ, json_type_object)) {
		AFB_DYNAPI_WARNING(apiHandle, "Response request object is not valid");
		return CALL_ERROR_REQUEST_NOT_VALID;
	}

	if(! json_object_object_get_ex(returnedRequestJ, "status", &returnedStatusJ)) {
		AFB_DYNAPI_WARNING(apiHandle, "Couldn't get response status object");
		return CALL_ERROR_REQUEST_STATUS_UNAVAILABLE;
	}

	if(! json_object_is_type(returnedStatusJ, json_type_string)) {
		AFB_DYNAPI_WARNING(apiHandle, "Response status object is not valid");
		return CALL_ERROR_REQUEST_STATUS_NOT_VALID;
	}

	*returnedStatus = (char *) json_object_get_string(returnedStatusJ);

	if(! strcmp(*returnedStatus, "unknown-api")) {
		AFB_DYNAPI_WARNING(apiHandle, "Api %s not found", apiCalled);
		return CALL_ERROR_API_UNAVAILABLE;
	}

	if(! strcmp(*returnedStatus, "unknown-verb")) {
		AFB_DYNAPI_WARNING(apiHandle, "Verb %s of api %s not found", verbCalled, apiCalled);
		return CALL_ERROR_VERB_UNAVAILABLE;
	}

	if(! json_object_object_get_ex(returnedRequestJ, "info", &returnedInfoJ)) {
		AFB_DYNAPI_WARNING(apiHandle, "Couldn't get response info object");
		return CALL_ERROR_REQUEST_INFO_UNAVAILABLE;
	}

	if(! json_object_is_type(returnedInfoJ, json_type_string)) {
		AFB_DYNAPI_WARNING(apiHandle, "Response info object is not valid");
		return CALL_ERROR_REQUEST_INFO_NOT_VALID;
	}

	*returnedInfo = (char *) json_object_get_string(returnedInfoJ);

	AFB_DYNAPI_WARNING(apiHandle,
			   "Api %s and verb %s found, but this error was raised : '%s' with this info : '%s'",
			   apiCalled,
			   verbCalled,
			   *returnedStatus,
			   *returnedInfo);

	return CALL_ERROR_RETURNED;
}

void HalUtlHandleAppFwCallErrorInRequest(afb_request *request, char *apiCalled, char *verbCalled, json_object *callReturnJ, char *errorStatusToSend)
{
	char **returnedStatus, **returnedInfo;

	afb_dynapi *apiHandle;

	if(! request || ! apiCalled || ! verbCalled || ! callReturnJ) {
		afb_request_fail_f(request, "invalid_args", "%s: invalid arguments", __func__);
		return;
	}

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail_f(request, "api_handle", "%s: Can't get hal manager api handle", __func__);
		return;
	}

	returnedStatus = alloca(sizeof(char *));
	returnedInfo = alloca(sizeof(char *));

	switch (HalUtlHandleAppFwCallError(apiHandle, apiCalled, verbCalled, callReturnJ, returnedStatus, returnedInfo)) {
		case CALL_ERROR_REQUEST_UNAVAILABLE:
		case CALL_ERROR_REQUEST_NOT_VALID:
		case CALL_ERROR_REQUEST_STATUS_UNAVAILABLE:
		case CALL_ERROR_REQUEST_STATUS_NOT_VALID:
		case CALL_ERROR_REQUEST_INFO_UNAVAILABLE:
		case CALL_ERROR_REQUEST_INFO_NOT_VALID:
			afb_request_fail(request, errorStatusToSend, "Error with response object");
			return;

		case CALL_ERROR_API_UNAVAILABLE:
			afb_request_fail_f(request, errorStatusToSend, "Api %s not found", apiCalled);
			return;

		case CALL_ERROR_VERB_UNAVAILABLE:
			afb_request_fail_f(request, errorStatusToSend, "Verb %s of api %s not found", verbCalled, apiCalled);
			return;

		case CALL_ERROR_RETURNED:
			afb_request_fail_f(request,
					   errorStatusToSend,
					   "Api %s and verb %s found, but this error was raised : '%s' with this info : '%s'",
					   apiCalled,
					   verbCalled,
					   *returnedStatus,
					   *returnedInfo);
			return;

		case CALL_ERROR_INVALID_ARGS:
		default:
			return;
	}
}