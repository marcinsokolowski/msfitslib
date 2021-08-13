#ifndef _CCDANAL_INTERFACE_H__
#define _CCDANAL_INTERFACE_H__

#include "ccddriver_interface.h"


#define CCD_ANAL_SOCKET_NAME "$(NDIR)/ccd_analyser_socket"
#define CCD_ANAL_INET_ADRES "127.0.0.1"
#define CCD_ANAL_PORT_NO    9734
#define CCD_MAX_ANSWER_BUFF_SIZE 4096


// REQUEST DEFINITION :

#define CCD_BEGIN_REQUEST "CCD_BEGIN_REQUEST"
#define CCD_END_REQUEST   "CCD_END_REQUEST"

// event types :
enum eCCDRequestType  { eCCDUndefined=0, eCCDSateliteTrigger, eCCDReset, 
								eCCDParamChange, eCCDExit, eCCDParamsRead };


#define CCD_SATELITE_TRIGGER "CCD_SATELITE_TRIGGER"
#define CCD_RESET            "CCD_RESET"
#define CCD_PARAM_CHANGE     "CCD_PARAM_CHANGE"
#define CCD_EXIT             "CCD_EXIT"
#define CCD_PARAMS_READ      "CCD_PARAMS_READ"


// request receipents :
enum eCCDRequestTo { eReqGlobal=0, eReqPipeline, eReqCamera };
#define CCD_PIPELINE_REQ "CCD_PIPELINE_REQ"
#define CCD_CAMERA_REQ   "CCD_CAMERA_REQ"


#endif
